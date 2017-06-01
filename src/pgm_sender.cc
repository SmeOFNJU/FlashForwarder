/**
 * VSS - Vendor Supplied System, used to receive real time quotes from 
 * Chinese market.
 * Copyright (c) 2015 hainan, ning (ninghainan@gmail.com).
 *
 * @version 1.0
 * @author hainan, ning
 */

#include <pgm_sender.h>

#include <boost/assert.hpp>

#include <select.h>
#include <pipe.h>
#include <message_allocator.h>

namespace vss {

static const int kOffsetBitCount = 13;
static const int kEncodeOptionBitCount = 5;
static const int kMaxMessagePerTime = 50;
static const uint16_t kNonExistOffset = 0x1FFF;
static const int kStatusReportInterval = 1;

PGMSender::PGMSender(Select* select, 
  const PGMOptions& options, int protocol, int compress_level,
  boost::shared_ptr<ISerializerFactory> serializer_factory,
  boost::shared_ptr<IIncrementSessionFactory> increment_factory,
  boost::shared_ptr<ILogger> logger) 
  : select_(select)
  , has_tx_timer_ (false)
  , has_rx_timer_(false)
  , pgm_socket_(false, options)
  , options_(options)
  , out_buffer_(NULL)
  , out_buffer_size_(0)
  , write_size_(0)
  , next_index_(0)
  , current_(NULL)
  , cur_size_(0)
  , current_offset_(0)
  , timestamp_(0)
  , cache_size_(0)
  , cache_msg_(NULL)
  , written_size_(0)
  , protocol_(protocol)
  , compress_level_(compress_level)
  , serializer_factory_(serializer_factory)
  , increment_factory_(increment_factory)
  , total_sent_(0)
  , total_bytes_sent_(0)
  , debug_enabled_(false)
  , total_delay_(0)
  , msg_count_(0)
  , logger_(logger) {
  offset_mask_ = static_cast<uint16_t>(compress_level << kOffsetBitCount);
  if (increment_factory) {
    offset_mask_ |= 1 << 15;
  }
}

PGMSender::~PGMSender() {
  if (out_buffer_) {
    free(out_buffer_);
    out_buffer_ = NULL;
  }
}

int PGMSender::Init(bool udp_encapsulation, const char *network) {
  int rc = pgm_socket_.Init(udp_encapsulation, network);
  if (rc != 0) {
    return rc;
  }
  
  out_buffer_size_ = pgm_socket_.GetMaxTsduSize();
  out_buffer_ = (unsigned char*) malloc(out_buffer_size_);
  BOOST_ASSERT(out_buffer_);
  cache_size_ = static_cast<int>(out_buffer_size_);
  // create pipes
  boost::shared_ptr<Pipe> pipes[2];
  VSS_CHECK_RESULT(CreatePipePair(pipes, 0));
  ui_pipe_ = pipes[0];
  io_pipe_ = pipes[1];
  // create the serializer
  serializer_ = serializer_factory_->Create(protocol_, "", "", false, 
    compress_level_, 0);
  if (increment_factory_) {
    increment_session_ = increment_factory_->Create(true);
  }
  return rc;
}

int PGMSender::Start() {
  // send command to let the select instance to plug this sender
  IOCommand cmd;
  cmd.type = IOCommand::TYPE_PLUG;
  cmd.engine = this;
  select_->command_handler().SendCommand(cmd);
  return 0;
}

void PGMSender::Stop() {
  // send command to let the select instance to terminate this sender
  IOCommand cmd;
  cmd.type = IOCommand::TYPE_TERMINATE;
  cmd.engine = this;
  select_->command_handler().SendCommand(cmd);
  // terminate the ui side pipe
  if (ui_pipe_) {
    ui_pipe_->Terminate();
    ui_pipe_.reset();
  }
}

int PGMSender::PushMessage(const uint8_t* content, int size) {
  if (NULL != cache_msg_) {
    // append to the cache message or flush the message
    if (size + cache_msg_->size() <= cache_size_) {
      memcpy_s(cache_msg_->buffer() + cache_msg_->size(), size, 
        content, size);
      cache_msg_->set_size(cache_msg_->size() + size);
      return 0;
    }
    // flush the message then create new cache
    bool awake_peer = false;
    int res = ui_pipe_->Write(cache_msg_, &awake_peer);
    if (0 != res) {
      ui_pipe_->ReleaseMessage(cache_msg_);
      cache_msg_ = NULL;
      return res;
    } else {
      if (awake_peer) {
        IOCommand cmd;
        cmd.type = IOCommand::TYPE_RESTART_OUTPUT;
        cmd.engine = this;
        select_->command_handler().SendCommand(cmd);
      }
    }
    cache_msg_ = NULL;
  }

  if (size >= cache_size_) {
    return DoWriteMessage(content, size);
  }
  // create the cache message
  VSS_CHECK_RESULT(ui_pipe_->allocator()->Allocate(cache_size_, 
    &cache_msg_));
  memcpy_s(cache_msg_->buffer(), cache_size_, content, size);
  cache_msg_->set_size(size);
  cache_msg_->set_timestamp(timestamp_);
  written_size_ += size;
  return 0;
}

int PGMSender::BeginPush(int64_t timestamp) {
  timestamp_ = timestamp;
  return 0;
}

int PGMSender::EndPush() {
  if (NULL != cache_msg_) {
    bool awake_peer = false;
    int res = ui_pipe_->Write(cache_msg_, &awake_peer);
    if (0 != res) {
      ui_pipe_->ReleaseMessage(cache_msg_);
      cache_msg_ = NULL;
      return res;
    } else {
      if (awake_peer) {
        IOCommand cmd;
        cmd.type = IOCommand::TYPE_RESTART_OUTPUT;
        cmd.engine = this;
        select_->command_handler().SendCommand(cmd);
      }
    }
    cache_msg_ = NULL;
  }
  return 0;
}

int PGMSender::DoWriteMessage(const uint8_t* content, int size) {
  Message* msg = NULL;
  VSS_CHECK_RESULT(ui_pipe_->allocator()->Allocate(size, &msg));
  memcpy_s(msg->buffer(), size, content, size);
  msg->set_size(size);
  msg->set_timestamp(timestamp_);
  bool awake_peer = false;
  int res = ui_pipe_->Write(msg, &awake_peer);
  if (0 != res) {
    ui_pipe_->ReleaseMessage(msg);
  } else {
    if (awake_peer) {
      IOCommand cmd;
      cmd.type = IOCommand::TYPE_RESTART_OUTPUT;
      cmd.engine = this;
      select_->command_handler().SendCommand(cmd);
    }
  }
  return res;
}

void PGMSender::Reset() {
  // nothing
}

void PGMSender::Plug() {
  //  Alocate 2 fds for PGM socket.
  fd_t downlink_socket_fd = retired_fd;
  fd_t uplink_socket_fd = retired_fd;
  fd_t rdata_notify_fd = retired_fd;
  fd_t pending_notify_fd = retired_fd;

  //  Fill fds from PGM transport and add them to the poller.
  pgm_socket_.GetSenderFds(&downlink_socket_fd, &uplink_socket_fd,
      &rdata_notify_fd, &pending_notify_fd);

  handle_ = select_->AddFd(downlink_socket_fd, this);
  uplink_handle_ = select_->AddFd(uplink_socket_fd, this);
  rdata_notify_handle_ = select_->AddFd(rdata_notify_fd, this);   
  pending_notify_handle_ = select_->AddFd(pending_notify_fd, this);

  //  Set POLLIN. We wont never want to stop polling for uplink = we never
  //  want to stop porocess NAKs.
  select_->SetPollin(uplink_handle_);
  select_->SetPollin(rdata_notify_handle_);
  select_->SetPollin(pending_notify_handle_);

  //  Set POLLOUT for downlink_socket_handle.
  select_->SetPollout(handle_);

  // report the status periodically
  select_->AddTimer(kStatusReportInterval * 1000, this, status_timter_id);
}

void PGMSender::Terminate() {
  if (has_rx_timer_) {
    select_->CancelTimer(this, rx_timer_id);
    has_rx_timer_ = false;
  }

  if (has_tx_timer_) {
    select_->CancelTimer(this, tx_timer_id);
    has_tx_timer_ = false;
  }
  select_->CancelTimer(this, status_timter_id);

  select_->RmFd(handle_);
  select_->RmFd(uplink_handle_);
  select_->RmFd(rdata_notify_handle_);
  select_->RmFd(pending_notify_handle_);

  if (io_pipe_) {
    io_pipe_->Terminate();
    io_pipe_.reset();
  }

  // show the delay on the console
  if (debug_enabled_) {
    if (msg_count_ > 0) {
      std::cout << "PGMSender:: avg delay: " 
        << total_delay_ / msg_count_ << std::endl;
    }
  }
}

void PGMSender::RestartInput() {
  BOOST_ASSERT(false);
}

void PGMSender::RestartOutput() {
  select_->SetPollout(handle_);
  OutEvent();
}

void PGMSender::InEvent() {
  if (has_rx_timer_) {
    select_->CancelTimer(this, rx_timer_id);
    has_rx_timer_ = false;
  }

  //  In-event on sender side means NAK or SPMR receiving from some peer.
  int rc = pgm_socket_.ProcessUpstream();
  if (rc == VSS_NOMEM || rc == VSS_BUSY) {
    const long timeout = pgm_socket_.GetRxTimeout();
    select_->AddTimer(timeout, this, rx_timer_id);
    has_rx_timer_ = true;
  }
}

void PGMSender::OutEvent() {
  //  POLLOUT event from send socket. If write buffer is empty, 
  //  try to read new data from the encoder.
  if (write_size_ == 0) {

    //  First two bytes (sizeof uint16_t) are used to store message 
    //  offset in following steps. Note that by passing our buffer to
    //  the get data function we prevent it from returning its own buffer.
    unsigned char *bf = out_buffer_ + sizeof (uint16_t) + sizeof(uint32_t);
    size_t bfsz = out_buffer_size_ - sizeof (uint16_t) - sizeof(uint32_t);
    uint16_t offset = kNonExistOffset | offset_mask_;

    size_t bytes = 0;
    while (bytes < bfsz) {
      if (NULL == current_) {
        bool avaiable = false;
        for (int i = 0; i < kMaxMessagePerTime; ++i) {
          Message* msg = NULL;
          int rc = io_pipe_->Read(0, &msg);
          if (0 != rc) {
            break;
          }
          if (debug_enabled_) {
            msg_count_ ++;
            int64_t start_time = GetCurrentMicroseconds();
            uint64_t delay = start_time - msg->timestamp();
            total_delay_ += delay;
          }
          avaiable = true;
          if (increment_session_) {
            increment_session_->OnIncrement(msg->buffer(), msg->size(), 
              serializer_.get());
          } else {
            serializer_->Serialize(msg->buffer(), msg->size());
          }
          io_pipe_->RecycleMessage(msg);
          if (kNonExistOffset == (offset & kNonExistOffset)) {
            offset = static_cast <uint16_t>(bytes) | offset_mask_;
          }
        }
        if (!avaiable) {
          // the data not read
          break;
        }
        serializer_->Flush(&current_, &cur_size_);
      }
      int wbytes = cur_size_ - current_offset_;
      int avaiable = static_cast<int>(bfsz - bytes);
      wbytes = wbytes > avaiable ? avaiable : wbytes;
      memcpy_s(bf + bytes, wbytes, current_ + current_offset_, 
        wbytes);
      current_offset_ += wbytes;
      bytes += wbytes;
      if (current_offset_ >= cur_size_) {
        current_ = NULL;
        current_offset_ = 0;
      }
    }

    //  If there are no data to write stop polling for output.
    if (bytes == 0) {
      select_->ResetPollout(handle_);
      return;
    }

    write_size_ = sizeof (uint16_t) + sizeof(uint32_t) + bytes;
    //  Put offset information in the buffer.
    *reinterpret_cast<uint16_t*>(out_buffer_) = offset;
    *reinterpret_cast<uint32_t*>(out_buffer_ + sizeof(uint16_t)) = 
      next_index_ ++;
  }

  if (has_tx_timer_) {
    select_->CancelTimer(this, tx_timer_id);
    select_->SetPollout(handle_);
    has_tx_timer_ = false;
  }

  //  Send the data.
  int nbytes = 0;
  int rc = pgm_socket_.Send(out_buffer_, write_size_, &nbytes);

  //  We can write either all data or 0 which means rate limit reached.
  if (nbytes == write_size_) {
    total_sent_ += write_size_;
    write_size_ = 0;
  } else {
    BOOST_ASSERT(nbytes == 0);

    if (rc == VSS_NOMEM) {
      // Stop polling handle and wait for tx timeout
      const long timeout = pgm_socket_.GetTxTimeout();
      select_->AddTimer (timeout, this, tx_timer_id);
      select_->ResetPollout(handle_);
      has_tx_timer_ = true;
    } else {
      BOOST_ASSERT(rc == VSS_BUSY);
    }
  }
}

void PGMSender::ReportStatus(PGMSendStatus* status) {
  status->total_bytes_sent = total_bytes_sent_.load(
    boost::memory_order_relaxed);
}

void PGMSender::TimerEvent(int token) {
  //  Timer cancels on return by poller_base.
  if (token == rx_timer_id) {
    has_rx_timer_ = false;
    InEvent();
  } else if (token == tx_timer_id) {
    // Restart polling handle and retry sending
    has_tx_timer_ = false;
    select_->SetPollout(handle_);
    OutEvent();
  } else if (token == status_timter_id) {
    // update the status
    total_bytes_sent_.store(total_sent_, boost::memory_order_relaxed);
    // report the status periodically
    select_->AddTimer(kStatusReportInterval * 1000, this, status_timter_id);
  } else {
    BOOST_ASSERT(false);
  }
}


} // namespace vss