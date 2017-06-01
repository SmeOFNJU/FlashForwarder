

#ifndef VSS_WIRE_FORMAT_H__
#define VSS_WIRE_FORMAT_H__
#include<stdint.h>
namespace vss {

// This class is for internal use by the  library, it contains helpers 
// for implementing the binary wire format.
//
// This class is really a namespace that contains only static methods.
class WireFormat {
 public:
  static const int kMaxVarintBytes = 10;
  static const int kMaxVarint32Bytes = 5;

  // Read a 16-bit little-endian integer.
  static void ReadLittleEndian16(const uint8_t* buffer, uint16_t* value);
  // Read a 32-bit little-endian integer.
  static void ReadLittleEndian32(const uint8_t* buffer, uint32_t* value);
  // Read a 64-bit little-endian integer.
  static void ReadLittleEndian64(const uint8_t* buffer, uint64_t* value);

  // Write a 16-bit little-endian integer.
  static void WriteLittleEndian16(uint16_t value, uint8_t* target);
  // Write a 32-bit little-endian integer.
  static void WriteLittleEndian32(uint32_t value, uint8_t* target);
  // Write a 64-bit little-endian integer.
  static void WriteLittleEndian64(uint64_t value, uint8_t* target);

  // Read a 16-bit big-endian integer.
  static void ReadBigEndian16(const uint8_t* buffer, uint16_t* value);
  // Read a 32-bit big-endian integer.
  static void ReadBigEndian32(const uint8_t* buffer, uint32_t* value);
  // Read a 64-bit big-endian integer.
  static void ReadBigEndian64(const uint8_t* buffer, uint64_t* value);

  // Write a 16-bit big-endian integer.
  static void WriteBigEndian16(uint16_t value, uint8_t* target);
  // Write a 32-bit big-endian integer.
  static void WriteBigEndian32(uint32_t value, uint8_t* target);
  // Write a 64-bit big-endian integer.
  static void WriteBigEndian64(uint64_t value, uint8_t* target);

  // Helper functions for converting between floats/doubles and IEEE-754
  // uint32s/uint64s so that they can be written.  (Assumes your platform
  // uses IEEE-754 floats.)
  static inline uint32_t EncodeFloat(float value);
  static inline float DecodeFloat(uint32_t value);
  static inline uint64_t EncodeDouble(double value);
  static inline double DecodeDouble(uint64_t value);

  // Helper functions for mapping signed integers to unsigned integers in
  // such a way that numbers with small magnitudes will encode to smaller
  // varints.  If you simply static_cast a negative number to an unsigned
  // number and varint-encode it, it will always take 10 bytes, defeating
  // the purpose of varint.  So, for the "sint32" and "sint64" field types,
  // we ZigZag-encode the values.
  static inline uint32_t ZigZagEncode32(int32_t n);
  static inline int32_t  ZigZagDecode32(uint32_t n);
  static inline uint64_t ZigZagEncode64(int64_t n);
  static inline int64_t  ZigZagDecode64(uint64_t n);

  // Write an unsigned integer with Varint encoding.  Writing a 32-bit value
  // is equivalent to casting it to uint64 and writing it as a 64-bit value,
  // but may be more efficient.
  static inline int WriteVarint32(uint32_t value, uint8_t* target);
  // Write an unsigned integer with Varint encoding.
  static inline int WriteVarint64(uint64_t value, uint8_t* target);
  
  static inline int WriteSignedVarint32(int32_t value, uint8_t* target);
  static inline int WriteSignedVarint64(int64_t value, uint8_t* target);

  // When parsing varints, we optimize for the common case of small values, and
  // then optimize for the case when the varint fits within the current buffer
  // piece. The Fallback method is used when we can't use the one-byte
  // optimization. The Slow method is yet another fallback when the buffer is
  // not large enough. Making the slow path out-of-line speeds up the common
  // case by 10-15%. The slow path is fairly uncommon: it only triggers when a
  // message crosses multiple buffers.
  static inline int ReadVarint32(const uint8_t* buffer, uint32_t* value);
  static inline int ReadVarint64(const uint8_t* buffer, uint64_t* value);
  static inline int ReadVarint32Safe(const uint8_t* buffer, int size, 
    uint32_t* value);
  static inline int ReadVarint64Safe(const uint8_t* buffer, int size, 
    uint64_t* value);

  static inline int ReadSignedVarint32(const uint8_t* buffer, int32_t* value);
  static inline int ReadSignedVarint64(const uint8_t* buffer, int64_t* value);

  // Returns the number of bytes needed to encode the given value as a varint.
  static int VarintSize32(uint32_t value);
  // Returns the number of bytes needed to encode the given value as a varint.
  static int VarintSize64(uint64_t value);

 private:
  // This class is not intend to be instanced
  WireFormat() {}

};

} // namespace vss

#endif // VSS_WIRE_FORMAT_H__