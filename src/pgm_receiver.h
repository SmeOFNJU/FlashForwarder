#ifndef VSS_PGM_RECEIVER_H__
#define VSS_PGM_RECEIVER_H__

#include <boost/atomic.hpp>

#include <pgm_socket.h>
#include <i_select_events.h>
#include <i_engine.h>
#include <pipe.h>
#include <i_logger.h>
#include <i_message_sink.h>
#include <i_message_parser.h>
#include <i_increment_apply.h>

namespace umdgw {

class Select;       // declare Select

/// Defines the pgm receive status
struct PGMReceiveStatus {
  uint64_t total_received_bytes;
  uint64_t total_received_msgs;
  uint64_t total_decoded_bytes;
  int64_t last_msg_timestamp;
  uint64_t total_missed_packets;

  PGMReceiveStatus()
    : total_received_bytes(0)
    , total_received_msgs(0)
    , total_decoded_bytes(0)
    , last_msg_timestamp(0)
    , total_missed_packets(0)
  {}

};

/// The pgm receiver
class PGMReceiver
  : public IMessageHandler
  , public ISelectEvents
  , public IEngine {
 public:
  PGMReceiver(Select* select, const PGMOptions& options, int protocol,
    boost::shared_ptr<IParserFactory> parser_factory,
    boost::shared_ptr<IIncrementApplyFactory> inc_apply_factory,
    boost::shared_ptr<ILogger> logger);
  virtual ~PGMReceiver();

  // init the receiver
  int Init(bool udp_encapsulation, const char *network);

  int Start();

  void Stop();

  void AddMessageSink(boost::shared_ptr<IMessageSink> sink) {
    sinks_.push_back(sink);
  }

  // IEngine interface implementation
  virtual void Plug();
  virtual void Terminate();
  virtual void RestartInput();
  virtual void RestartOutput();

  // ISelectEvents interface implementation.
  virtual void InEvent();
  virtual void OutEvent();
  virtual void TimerEvent(int id);

  // Reports the current status
  void ReportStatus(PGMReceiveStatus* status);

 private:
  //  RX timeout timer ID.
  enum {rx_timer_id = 0xa1, status_timter_id = 0x55};

  // unplug the receiver
  void Unplug();

  /// Handles the specified message.
  virtual void HandleMessage(int type, const uint8_t* content, int size);

  // help method to clear peers
  void DoClearPeers();

  // the owner select object
  Select* select_;

  //  RX timer is running.
  bool has_rx_timer_;

  //  If joined is true we are already getting messages from the peer.
  //  It it's false, we are getting data but still we haven't seen
  //  beginning of a message.
  struct peer_info_t {
    peer_info_t() 
      : joined(false)
      , begin_index(0)
      , last_index(0)
      , received_cnt(0)
      , missed_cnt(0) {}

    bool joined;
    uint32_t begin_index;
    uint32_t last_index;
    uint32_t received_cnt;
    uint32_t missed_cnt;
    boost::shared_ptr<IMessageParser> parser;
    boost::shared_ptr<IIncrementApply> inc_apply;
  };

  struct tsi_comp {
    bool operator () (const pgm_tsi_t &ltsi, const pgm_tsi_t &rtsi) const {
      uint32_t ll[2], rl[2];
      memcpy (ll, &ltsi, sizeof (ll));
      memcpy (rl, &rtsi, sizeof (rl));
      return (ll[0] < rl[0]) || (ll[0] == rl[0] && ll[1] < rl[1]);
    }
  };

  /// define the atomic status
  struct AtomicStatus {
    boost::atomic_uint64_t total_received_bytes;
    boost::atomic_uint64_t total_received_msgs;
    boost::atomic_uint64_t total_decoded_bytes;
    boost::atomic_int64_t last_msg_timestamp;
    boost::atomic_uint64_t total_missed_packets;

    AtomicStatus()
      : total_received_bytes(0)
      , total_received_msgs(0)
      , total_decoded_bytes(0)
      , last_msg_timestamp(0)
    {}
  };

  typedef std::map<pgm_tsi_t, peer_info_t*, tsi_comp> peers_t;
  peers_t peers_;

  //  PGM socket.
  PGMSocket pgm_socket_;

  //  Socket options.
  PGMOptions options_;

  const pgm_tsi_t *active_tsi_;

  //  Number of bytes not consumed by the decoder due to pipe overflow.
  size_t insize_;

  //  Pointer to data still waiting to be processed by the decoder.
  const unsigned char *inpos_;

  //  Poll handle associated with PGM socket.
  fd_t socket_handle_;

  //  Poll handle associated with engine PGM waiting pipe.
  fd_t pipe_handle_;

  // the protocol
  int protocol_;
  // the message sinks
  std::vector<boost::shared_ptr<IMessageSink> > sinks_;
  // the parser factory
  boost::shared_ptr<IParserFactory> parser_factory_;
  boost::shared_ptr<IIncrementApplyFactory> inc_apply_factory_;
  // the logger
  boost::shared_ptr<ILogger> logger_;

  IIncrementApply* current_inc_apply_;
  // the current status, only used internally by the i/o thread
  PGMReceiveStatus status_;
  // the current status of the receiver, it would be updated periodically
  AtomicStatus atomic_status_;

};

} // namespace umdgw

#endif // VSS_PGM_RECEIVER_H__