#include <pgm_receiver.h>

#include <boost/date_time.hpp>

#include <select.h>
#include <message_allocator.h>

namespace umdgw {

static const int kOffsetBitCount = 13;
static const int kEncodeOptionBitCount = 5;
static const uint16_t kNonExistOffset = 0x1FFF;
static const int kStatusReportInterval = 1;

PGMReceiver::PGMReceiver(
  Select* select, const PGMOptions& options, int protocol,
  boost::shared_ptr<IParserFactory> parser_factory,
  boost::shared_ptr<IIncrementApplyFactory> inc_apply_factory,
  boost::shared_ptr<ILogger> logger)
  : select_(select)
  , has_rx_timer_(false)
  , pgm_socket_(true, options)
  , options_(options)
  , active_tsi_(NULL)
  , insize_ (0)
  , inpos_(NULL)
  , parser_factory_(parser_factory)
  , inc_apply_factory_(inc_apply_factory)
  , current_inc_apply_(NULL)
  , protocol_(protocol)
  , logger_(logger) {
  // nothing
}

PGMReceiver::~PGMReceiver() {
  //  Destructor should not be called before unplug.
  BOOST_ASSERT(peers_.empty());
}

int PGMReceiver::Init(bool udp_encapsulation, const char *network) {
  int rc = pgm_socket_.Init(udp_encapsulation, network);
  return rc;
}

int PGMReceiver::Start() {
  for (size_t i = 0; i < sinks_.size(); ++i) {
    VSS_CHECK_RESULT(sinks_[i]->Start());
  }
  // send command to let the select instance to plug this sender
  IOCommand cmd;
  cmd.type = IOCommand::TYPE_PLUG;
  cmd.engine = this;
  IOCommandHandler* handler = &select_->command_handler();
  handler->SendCommand(cmd);
  return 0;
}

void PGMReceiver::Stop() {
  // send command to let the select instance to terminate this sender
  IOCommand cmd;
  cmd.type = IOCommand::TYPE_TERMINATE;
  cmd.engine = this;
  select_->command_handler().SendCommand(cmd);
}

void PGMReceiver::Plug() {
  //  Retrieve PGM fds and start polling.
  fd_t socket_fd = retired_fd;
  fd_t waiting_pipe_fd = retired_fd;
  pgm_socket_.GetReceiverFds(&socket_fd, &waiting_pipe_fd);
  socket_handle_ = select_->AddFd(socket_fd, this);
  pipe_handle_ = select_->AddFd(waiting_pipe_fd, this);
  select_->SetPollin(pipe_handle_);
  select_->SetPollin(socket_handle_);

  // report the status periodically
  select_->AddTimer(kStatusReportInterval * 1000, this, status_timter_id);
}

void PGMReceiver::Terminate() {
  DoClearPeers();
  active_tsi_ = NULL;

  if (has_rx_timer_) {
    select_->CancelTimer(this, rx_timer_id);
    has_rx_timer_ = false;
  }
  select_->CancelTimer(this, status_timter_id);

  select_->RmFd(socket_handle_);
  select_->RmFd(pipe_handle_);

  for (size_t i = 0; i < sinks_.size(); ++i) {
    sinks_[i]->Stop();
  }
}

void PGMReceiver::Unplug() {
  for (peers_t::iterator it = peers_.begin (); it != peers_.end (); ++it) {
    it->second->joined = false;
    it->second->parser.reset();
  }
  active_tsi_ = NULL;

  if (has_rx_timer_) {
    select_->CancelTimer(this, rx_timer_id);
    has_rx_timer_ = false;
  }

  select_->RmFd(socket_handle_);
  select_->RmFd(pipe_handle_);

  insize_ = 0;
  inpos_ = 0;
  socket_handle_ = 0;
  pipe_handle_ = 0;
}

void PGMReceiver::RestartInput() {
  BOOST_ASSERT(false);
}

void PGMReceiver::RestartOutput() {
  BOOST_ASSERT(false);
}

void PGMReceiver::InEvent() {
  // Read data from the underlying pgm_socket.
  const pgm_tsi_t *tsi = NULL;

  if (has_rx_timer_) {
    select_->CancelTimer(this, rx_timer_id);
    has_rx_timer_ = false;
  }

  //  This loop can effectively block other engines in the same I/O
  //  thread in the case of high load.
  while (true) {

    //  Get new batch of data.
    //  Note the workaround made not to break strict-aliasing rules.
    void *tmp = NULL;
    int received = 0;
    int rc = pgm_socket_.Receive(&tmp, &tsi, &received);
    inpos_ = (unsigned char*) tmp;

    //  No data to process. This may happen if the packet received is
    //  neither ODATA nor ODATA.
    if (received == 0) {
      if (rc == VSS_NOMEM || rc == VSS_BUSY) {
        const long timeout = pgm_socket_.GetRxTimeout();
        select_->AddTimer(timeout, this, rx_timer_id);
        has_rx_timer_ = true;
      }
      break;
    }

    //  Data loss. Delete decoder and mark the peer as disjoint.
    if (received == -1) {
      // we just reset the socket here to fix the bug that no more messages passed to app
      /*Unplug();
      pgm_socket_.Reset();
      Plug();*/
      /*if (it != peers_.end ()) {
        it->second->joined = false;
        it->second->parser.reset();
      }*/
      /*for (peers_t::iterator it = peers_.begin (); it != peers_.end (); ++it) {
        it->second->joined = false;
        it->second->parser.reset();
      }
      std::cout << "Data lost!" << std::endl;*/
      break;
    }

    //  Find the peer based on its TSI.
    peers_t::iterator it = peers_.find(*tsi);

    insize_ = static_cast <size_t>(received);
    status_.total_received_bytes += received;

    //  Read the offset of the fist message in the current packet.
    BOOST_ASSERT(insize_ >= sizeof (uint16_t) + sizeof(uint32_t));
    uint16_t tvalue = *(reinterpret_cast<const uint16_t*>(inpos_));
    uint16_t offset = tvalue & kNonExistOffset;
    int compress_level = tvalue >> kOffsetBitCount;
    bool inc_update = (compress_level & (1 << 2)) != 0;
    compress_level &= ~(1 << 2);
    uint32_t index = *(reinterpret_cast<const uint32_t*>(inpos_ + 2));
    inpos_ += sizeof (uint16_t) + sizeof(uint32_t);
    insize_ -= sizeof (uint16_t) + sizeof(uint32_t);

    //  New peer. Add it to the list of know but unjoint peers.
    if (it == peers_.end()) {
      // clear the peers since we only expect one active
      DoClearPeers();
      peer_info_t* peer = new peer_info_t();
      it = peers_.insert(peers_t::value_type (*tsi, peer)).first;
      it->second->begin_index = index;  
    } else {
      if (0 != it->second->last_index && it->second->last_index + 1 != index) {
        it->second->joined = false;
        it->second->last_index = 0;
        it->second->parser.reset();
        it->second->inc_apply.reset();
        int missed = 0;
        if (index > it->second->last_index) {
          missed = index - it->second->last_index - 1;
        } else {
          missed = std::numeric_limits<uint32_t>::max() - 
            it->second->last_index + index - 1;
        }
        it->second->missed_cnt += missed;
        status_.total_missed_packets += missed;
      }
      // save current index
      it->second->last_index = index;
    }
    it->second->received_cnt += 1;

    //  Join the stream if needed.
    if (!it->second->joined) {
      //  There is no beginning of the message in current packet.
      //  Ignore the data.
      if (offset == kNonExistOffset) {
        continue;
      }
      
      BOOST_ASSERT(offset <= insize_);

      //  We have to move data to the begining of the first message.
      inpos_ += offset;
      insize_ -= offset;

      //  Mark the stream as joined.
      it->second->joined = true;
      //  Create the parser for the peer.
      it->second->parser = parser_factory_->Create(protocol_, false, 
        compress_level, 0, this);
      if (inc_update && inc_apply_factory_) {
        it->second->inc_apply = inc_apply_factory_->Create();
      }
    }

    int64_t timestamp = GetCurrentMicroseconds();

    status_.last_msg_timestamp = timestamp;
    for (size_t i = 0; i < sinks_.size(); ++i) {
      sinks_[i]->BeginPush(timestamp);
    }

    current_inc_apply_ = it->second->inc_apply.get();
    int ret = it->second->parser->Parse(inpos_, inpos_ + insize_);
    if (0 != ret) {
      it->second->joined = false;
      it->second->last_index = 0;
      it->second->parser.reset();
      it->second->inc_apply.reset();
      inpos_ = NULL;
      insize_ = 0;
    }

    for (size_t i = 0; i < sinks_.size(); ++i) {
      sinks_[i]->EndPush();
    }
  }
}

void PGMReceiver::OutEvent() {
  BOOST_ASSERT(false);
}

void PGMReceiver::TimerEvent(int token) {
  if (token == rx_timer_id) {
    //  Timer cancels on return by poller_base.
    has_rx_timer_ = false;
    InEvent();
  } else if (token == status_timter_id) {
    // update the status
    atomic_status_.total_received_bytes.store(status_.total_received_bytes, 
      boost::memory_order_relaxed);
    atomic_status_.total_received_msgs.store(status_.total_received_msgs, 
      boost::memory_order_relaxed);
    atomic_status_.total_decoded_bytes.store(status_.total_decoded_bytes, 
      boost::memory_order_relaxed);
    atomic_status_.last_msg_timestamp.store(status_.last_msg_timestamp, 
      boost::memory_order_relaxed);
    atomic_status_.total_missed_packets.store(status_.total_missed_packets, 
      boost::memory_order_relaxed);
    // report the status periodically
    select_->AddTimer(kStatusReportInterval * 1000, this, status_timter_id);
  } else {
    BOOST_ASSERT(false);
  }
}

void PGMReceiver::ReportStatus(PGMReceiveStatus* status) {
  status->total_missed_packets = atomic_status_.total_missed_packets.load(
    boost::memory_order_relaxed);
  status->total_received_bytes = atomic_status_.total_received_bytes.load(
    boost::memory_order_relaxed);
  status->total_received_msgs = atomic_status_.total_received_msgs.load(
    boost::memory_order_relaxed);
  status->total_decoded_bytes = atomic_status_.total_decoded_bytes.load(
    boost::memory_order_relaxed);
  status->last_msg_timestamp = atomic_status_.last_msg_timestamp.load(
    boost::memory_order_relaxed);
}

void PGMReceiver::HandleMessage(int type, const uint8_t* content, int size) {
  if (current_inc_apply_) {
    const uint8_t* raw = NULL;
    int raw_size = 0;
    current_inc_apply_->Apply(content, size, &raw, &raw_size);
    if (raw_size > 0) {
      for (size_t i = 0; i < sinks_.size(); ++i) {
        sinks_[i]->PushMessage(raw, raw_size);
      }
      status_.total_decoded_bytes += raw_size;
    } else {
      status_.total_decoded_bytes += size;
    }
  } else {
    for (size_t i = 0; i < sinks_.size(); ++i) {
      sinks_[i]->PushMessage(content, size);
    }
    status_.total_decoded_bytes += size;
  }

  status_.total_received_msgs += 1;
}

void PGMReceiver::DoClearPeers() {
  //  Delete peers.
  for (peers_t::iterator it = peers_.begin(); it != peers_.end(); ++it) {
    //std::cout << "Missed count: " << it->second->missed_cnt << std::endl;
    delete it->second;
  }
  peers_.clear();
}

} // namespace umdgw