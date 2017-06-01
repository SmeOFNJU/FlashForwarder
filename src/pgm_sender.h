
#ifndef VSS_PGM_SENDER_H__
#define VSS_PGM_SENDER_H__

#include <boost/atomic.hpp>

#include <i_message_sink.h>
#include <pgm_socket.h>
#include <i_select_events.h>
#include <i_engine.h>
#include <pipe.h>
#include <i_logger.h>
#include <i_message_serializer.h>
#include <i_increment_update.h>

namespace umdgw {

class Select;       // declare Select

struct PGMSendStatus {
  uint64_t total_bytes_sent;

  PGMSendStatus()
    : total_bytes_sent(0)
  {}
};

class PGMSender 
  : public IMessageSink
  , public ISelectEvents
  , public IEngine {
 public:
  PGMSender(Select* select, const PGMOptions& options, 
    int protocol, int compress_level,
    boost::shared_ptr<ISerializerFactory> serializer_factory,
    boost::shared_ptr<IIncrementSessionFactory> increment_factory,
    boost::shared_ptr<ILogger> logger);
  virtual ~PGMSender();

  // init the sender
  int Init(bool udp_encapsulation, const char *network);

  // implement IMessageSink interface
  virtual int Start();
  virtual void Stop();
  virtual int BeginPush(int64_t timestamp);
  virtual int PushMessage(const uint8_t* content, int size);
  virtual int EndPush();
  virtual void Reset();

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
  void ReportStatus(PGMSendStatus* status);

  // for test purpose only
  void set_debug_enabled(bool enabled) {
    debug_enabled_ = enabled;
  }

 private:
  //  TX and RX timeout timer ID's.
  enum {tx_timer_id = 0xa0, rx_timer_id = 0xa1, status_timter_id = 0x55};

  // write the message to the pipe
  int DoWriteMessage(const uint8_t* content, int size);

  // the current timestamp
  int64_t timestamp_;
  // the max cache size
  int cache_size_;
  // the message holding cache data
  Message* cache_msg_;

  // the owner select object
  Select* select_;

  //  Timers are running.
  bool has_tx_timer_;
  bool has_rx_timer_;

  //  PGM socket.
  PGMSocket pgm_socket_;

  //  Socket options.
  PGMOptions options_;

  //  Poll handle associated with PGM socket.
  fd_t handle_;
  fd_t uplink_handle_;
  fd_t rdata_notify_handle_;
  fd_t pending_notify_handle_;

  // index of the next message to send
  uint32_t next_index_;

  //  Output buffer from pgm_socket.
  unsigned char *out_buffer_;

  //  Output buffer size.
  size_t out_buffer_size_;

  //  Number of bytes in the buffer to be written to the socket.
  //  If zero, there are no data to be sent.
  size_t write_size_;

  // the current buffer being encoded
  const uint8_t* current_;
  int cur_size_;
  // send offset in current buffer
  int current_offset_;

  // the statistics
  volatile int64_t written_size_;

  // pipes to pass messages
  boost::shared_ptr<Pipe> ui_pipe_;
  boost::shared_ptr<Pipe> io_pipe_;

  // the compress level
  int compress_level_;
  int protocol_;
  // the serializer factory
  boost::shared_ptr<ISerializerFactory> serializer_factory_;
  uint16_t offset_mask_;
  // the serializer
  boost::shared_ptr<IMessageSerializer> serializer_;
  boost::shared_ptr<IIncrementSessionFactory> increment_factory_;
  boost::shared_ptr<IIncrementSession> increment_session_;

  uint64_t total_sent_;
  boost::atomic_uint64_t total_bytes_sent_;

  // for performance debug
  bool debug_enabled_;
  uint64_t total_delay_;
  uint64_t msg_count_;

  // the logger
  boost::shared_ptr<ILogger> logger_;

};

} // namespace umdgw

#endif // VSS_PGM_SENDER_H__