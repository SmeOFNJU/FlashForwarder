#ifndef VSS_PGM_SOCKET_H__
#define VSS_PGM_SOCKET_H__

#ifdef _WIN32
#include <windows_ref.h>
#include <stdint.h>
#define __PGM_WININT_H__
#else
#include <poll.h>
#endif

#include <pgm/pgm.h>

#include <fd.h>
#include <pgm_options.h>
#include <common.h>

namespace umdgw {

//  Encapsulates PGM socket.
class PGMSocket {
 public:

  //  If receiver_ is true PGM transport is not generating SPM packets.
  PGMSocket(bool receiver, const PGMOptions& options);
  //  Closes the transport.
  ~PGMSocket();

  //  Initialize PGM network structures (GSI, GSRs).
  int Init(bool udp_encapsulation, const char *network);

  //  Resolve PGM socket address.
  static int InitAddress(const char *network, struct pgm_addrinfo_t **addr, 
    uint16_t *port_number);
  
  // reset the socket
  int Reset();

  //   Get receiver fds and store them into user allocated memory.
  void GetReceiverFds(fd_t *receive_fd, fd_t *waiting_pipe_fd);

  //   Get sender and receiver fds and store it to user allocated 
  //   memory. Receive fd is used to process NAKs from peers.
  void GetSenderFds(fd_t *send_fd, fd_t *receive_fd,
    fd_t *rdata_notify_fd, fd_t *pending_notify_fd);

  //  Send data as one APDU, transmit window owned memory.
  int Send(unsigned char *data, size_t data_len, int* o_sent_bytes);

  //  Returns max tsdu size without fragmentation.
  size_t GetMaxTsduSize();

  //  Receive data from pgm socket.
  int Receive(void **data, const pgm_tsi_t **tsi, int* o_received_bytes);

  long GetRxTimeout();
  long GetTxTimeout();

  //  POLLIN on sender side should mean NAK or SPMR receiving. 
  //  process_upstream function is used to handle such a situation.
  int ProcessUpstream();

 private:

  //  Compute size of the buffer based on rate and recovery interval.
  int ComputeSqns(int tpdu);

  //  OpenPGM transport.
  pgm_sock_t* sock_;

  int last_rx_status_;
  int last_tx_status_;

  //  Associated socket options.
  PGMOptions options_;

  //  true when pgm_socket should create receiving side.
  bool receiver_;

  //  Array of pgm_msgv_t structures to store received data
  //  from the socket (pgm_transport_recvmsgv).
  pgm_msgv_t* pgm_msgv_;

  //  Size of pgm_msgv array.
  size_t pgm_msgv_len_;

  // How many bytes were read from pgm socket.
  size_t nbytes_rec_;

  //  How many bytes were processed from last pgm socket read.
  size_t nbytes_processed_;
  
  //  How many messages from pgm_msgv were already sent up.
  size_t pgm_msgv_processed_;

  // the init parameters
  bool udp_encapsulation_;
  std::string network_;


};

} // namespace umdgw

#endif // VSS_PGM_SOCKET_H__