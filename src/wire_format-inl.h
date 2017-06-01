
#ifndef VSS_WIRE_FORMAT_INI_H__
#define VSS_WIRE_FORMAT_INI_H__

#include "wire_format.h"
#include<stdint.h>

namespace vss {

inline uint32_t WireFormat::EncodeFloat(float value) {
  union {float f; uint32_t i;};
  f = value;
  return i;
}

inline float WireFormat::DecodeFloat(uint32_t value) {
  union {float f; uint32_t i;};
  i = value;
  return f;
}

inline uint64_t WireFormat::EncodeDouble(double value) {
  union {double f; uint64_t i;};
  f = value;
  return i;
}

inline double WireFormat::DecodeDouble(uint64_t value) {
  union {double f; uint64_t i;};
  i = value;
  return f;
}

// ZigZag Transform:  Encodes signed integers so that they can be
// effectively used with varint encoding.
//
// varint operates on unsigned integers, encoding smaller numbers into
// fewer bytes.  If you try to use it on a signed integer, it will treat
// this number as a very large unsigned integer, which means that even
// small signed numbers like -1 will take the maximum number of bytes
// (10) to encode.  ZigZagEncode() maps signed integers to unsigned
// in such a way that those with a small absolute value will have smaller
// encoded values, making them appropriate for encoding using varint.
//
//       int32 ->     uint32
// -------------------------
//           0 ->          0
//          -1 ->          1
//           1 ->          2
//          -2 ->          3
//         ... ->        ...
//  2147483647 -> 4294967294
// -2147483648 -> 4294967295
//
//        >> encode >>
//        << decode <<

inline uint32_t WireFormat::ZigZagEncode32(int32_t n) {
  // Note:  the right-shift must be arithmetic
  return (n << 1) ^ (n >> 31);
}

inline int32_t WireFormat::ZigZagDecode32(uint32_t n) {
  return (n >> 1) ^ -static_cast<int32_t>(n & 1);
}

inline uint64_t WireFormat::ZigZagEncode64(int64_t n) {
  // Note:  the right-shift must be arithmetic
  return (n << 1) ^ (n >> 63);
}

inline int64_t WireFormat::ZigZagDecode64(uint64_t n) {
  return (n >> 1) ^ -static_cast<int64_t>(n & 1);
}

inline int WireFormat::WriteVarint32(uint32_t value, uint8_t* target) {
  target[0] = static_cast<uint8_t>(value | 0x80);
  if (value >= (1 << 7)) {
    target[1] = static_cast<uint8_t>((value >>  7) | 0x80);
    if (value >= (1 << 14)) {
      target[2] = static_cast<uint8_t>((value >> 14) | 0x80);
      if (value >= (1 << 21)) {
        target[3] = static_cast<uint8_t>((value >> 21) | 0x80);
        if (value >= (1 << 28)) {
          target[4] = static_cast<uint8_t>(value >> 28);
          return 5;
        } else {
          target[3] &= 0x7F;
          return 4;
        }
      } else {
        target[2] &= 0x7F;
        return 3;
      }
    } else {
      target[1] &= 0x7F;
      return 2;
    }
  } else {
    target[0] &= 0x7F;
    return 1;
  }
}

inline int WireFormat::WriteVarint64(uint64_t value, uint8_t* target) {
  // Splitting into 32-bit pieces gives better performance on 32-bit
  // processors.
  uint32_t part0 = static_cast<uint32_t>(value);
  uint32_t part1 = static_cast<uint32_t>(value >> 28);
  uint32_t part2 = static_cast<uint32_t>(value >> 56);

  int size;

  // Here we can't really optimize for small numbers, since the value is
  // split into three parts. Checking for numbers < 128, for instance,
  // would require three comparisons, since you'd have to make sure part1
  // and part2 are zero.  However, if the caller is using 64-bit integers,
  // it is likely that they expect the numbers to often be very large, so
  // we probably don't want to optimize for small numbers anyway.  Thus,
  // we end up with a hard coded binary search tree...
  if (part2 == 0) {
    if (part1 == 0) {
      if (part0 < (1 << 14)) {
        if (part0 < (1 << 7)) {
          size = 1; goto size1;
        } else {
          size = 2; goto size2;
        }
      } else {
        if (part0 < (1 << 21)) {
          size = 3; goto size3;
        } else {
          size = 4; goto size4;
        }
      }
    } else {
      if (part1 < (1 << 14)) {
        if (part1 < (1 << 7)) {
          size = 5; goto size5;
        } else {
          size = 6; goto size6;
        }
      } else {
        if (part1 < (1 << 21)) {
          size = 7; goto size7;
        } else {
          size = 8; goto size8;
        }
      }
    }
  } else {
    if (part2 < (1 << 7)) {
      size = 9; goto size9;
    } else {
      size = 10; goto size10;
    }
  }

  size10: target[9] = static_cast<uint8_t>((part2 >>  7) | 0x80);
  size9 : target[8] = static_cast<uint8_t>((part2      ) | 0x80);
  size8 : target[7] = static_cast<uint8_t>((part1 >> 21) | 0x80);
  size7 : target[6] = static_cast<uint8_t>((part1 >> 14) | 0x80);
  size6 : target[5] = static_cast<uint8_t>((part1 >>  7) | 0x80);
  size5 : target[4] = static_cast<uint8_t>((part1      ) | 0x80);
  size4 : target[3] = static_cast<uint8_t>((part0 >> 21) | 0x80);
  size3 : target[2] = static_cast<uint8_t>((part0 >> 14) | 0x80);
  size2 : target[1] = static_cast<uint8_t>((part0 >>  7) | 0x80);
  size1 : target[0] = static_cast<uint8_t>((part0      ) | 0x80);

  target[size-1] &= 0x7F;
  return size;
}

inline int WireFormat::WriteSignedVarint32(int32_t value, uint8_t* target) {
  return WriteVarint32(ZigZagEncode32(value), target);
}

inline int WireFormat::WriteSignedVarint64(int64_t value, uint8_t* target) {
  return WriteVarint64(ZigZagEncode64(value), target);
}

inline int WireFormat::ReadVarint32(const uint8_t* buffer, uint32_t* value) {
  // Fast path:  We have enough bytes left in the buffer to guarantee that
  // this read won't cross the end, so we can skip the checks.
  const uint8_t* ptr = buffer;
  uint32_t b;
  uint32_t result;

  b = *(ptr++); result  = b      ; if (!(b & 0x80)) goto done;
  result -= 0x80;
  b = *(ptr++); result += b <<  7; if (!(b & 0x80)) goto done;
  result -= 0x80 << 7;
  b = *(ptr++); result += b << 14; if (!(b & 0x80)) goto done;
  result -= 0x80 << 14;
  b = *(ptr++); result += b << 21; if (!(b & 0x80)) goto done;
  result -= 0x80 << 21;
  b = *(ptr++); result += b << 28; if (!(b & 0x80)) goto done;
  // "result -= 0x80 << 28" is irrevelant.

  // We have overrun the maximum size of a varint32 (5 bytes).  Assume
  // the data is corrupt.
  return -1;

done:
  *value = result;
  return static_cast<int>(ptr - buffer);
}

inline int WireFormat::ReadVarint64(const uint8_t* buffer, uint64_t* value) {
  const uint8_t* ptr = buffer;
  uint32_t b;

  // Splitting into 32-bit pieces gives better performance on 32-bit
  // processors.
  uint32_t part0 = 0, part1 = 0, part2 = 0;

  b = *(ptr++); part0  = b      ; if (!(b & 0x80)) goto done;
  part0 -= 0x80;
  b = *(ptr++); part0 += b <<  7; if (!(b & 0x80)) goto done;
  part0 -= 0x80 << 7;
  b = *(ptr++); part0 += b << 14; if (!(b & 0x80)) goto done;
  part0 -= 0x80 << 14;
  b = *(ptr++); part0 += b << 21; if (!(b & 0x80)) goto done;
  part0 -= 0x80 << 21;
  b = *(ptr++); part1  = b      ; if (!(b & 0x80)) goto done;
  part1 -= 0x80;
  b = *(ptr++); part1 += b <<  7; if (!(b & 0x80)) goto done;
  part1 -= 0x80 << 7;
  b = *(ptr++); part1 += b << 14; if (!(b & 0x80)) goto done;
  part1 -= 0x80 << 14;
  b = *(ptr++); part1 += b << 21; if (!(b & 0x80)) goto done;
  part1 -= 0x80 << 21;
  b = *(ptr++); part2  = b      ; if (!(b & 0x80)) goto done;
  part2 -= 0x80;
  b = *(ptr++); part2 += b <<  7; if (!(b & 0x80)) goto done;
  // "part2 -= 0x80 << 7" is irrelevant because (0x80 << 7) << 56 is 0.

  // We have overrun the maximum size of a varint (10 bytes).  The data
  // must be corrupt.
  return -1;

done:
  *value = (static_cast<uint64_t>(part0)) |
    (static_cast<uint64_t>(part1) << 28) |
    (static_cast<uint64_t>(part2) << 56);
  return static_cast<int>(ptr - buffer);
}

inline int WireFormat::ReadVarint32Safe(const uint8_t* buffer, int size, 
  uint32_t* value) {
  const uint8_t* ptr = buffer;
  uint32_t b;
  uint32_t result;

  if (size <= 0) return -1;
  b = *(ptr++); result  = b      ; if (!(b & 0x80)) goto done;
  if (size == 1) return -1;
  result -= 0x80;
  b = *(ptr++); result += b <<  7; if (!(b & 0x80)) goto done;
  if (size == 2) return -1;
  result -= 0x80 << 7;
  b = *(ptr++); result += b << 14; if (!(b & 0x80)) goto done;
  if (size == 3) return -1;
  result -= 0x80 << 14;
  b = *(ptr++); result += b << 21; if (!(b & 0x80)) goto done;
  if (size == 4) return -1;
  result -= 0x80 << 21;
  b = *(ptr++); result += b << 28; if (!(b & 0x80)) goto done;
  // "result -= 0x80 << 28" is irrevelant.

  // We have overrun the maximum size of a varint32 (5 bytes).  Assume
  // the data is corrupt.
  return -1;

done:
  *value = result;
  return static_cast<int>(ptr - buffer);
}

inline int WireFormat::ReadVarint64Safe(const uint8_t* buffer, int size, 
  uint64_t* value) {
  const uint8_t* ptr = buffer;
  uint32_t b;

  // Splitting into 32-bit pieces gives better performance on 32-bit
  // processors.
  uint32_t part0 = 0, part1 = 0, part2 = 0;

  if (size <= 0) return -1;
  b = *(ptr++); part0  = b      ; if (!(b & 0x80)) goto done;
  if (size == 1) return -1;
  part0 -= 0x80;
  b = *(ptr++); part0 += b <<  7; if (!(b & 0x80)) goto done;
  if (size == 2) return -1;
  part0 -= 0x80 << 7;
  b = *(ptr++); part0 += b << 14; if (!(b & 0x80)) goto done;
  if (size == 3) return -1;
  part0 -= 0x80 << 14;
  b = *(ptr++); part0 += b << 21; if (!(b & 0x80)) goto done;
  if (size == 4) return -1;
  part0 -= 0x80 << 21;
  b = *(ptr++); part1  = b      ; if (!(b & 0x80)) goto done;
  if (size == 5) return -1;
  part1 -= 0x80;
  b = *(ptr++); part1 += b <<  7; if (!(b & 0x80)) goto done;
  if (size == 6) return -1;
  part1 -= 0x80 << 7;
  b = *(ptr++); part1 += b << 14; if (!(b & 0x80)) goto done;
  if (size == 7) return -1;
  part1 -= 0x80 << 14;
  b = *(ptr++); part1 += b << 21; if (!(b & 0x80)) goto done;
  if (size == 8) return -1;
  part1 -= 0x80 << 21;
  b = *(ptr++); part2  = b      ; if (!(b & 0x80)) goto done;
  if (size == 9) return -1;
  part2 -= 0x80;
  b = *(ptr++); part2 += b <<  7; if (!(b & 0x80)) goto done;
  // "part2 -= 0x80 << 7" is irrelevant because (0x80 << 7) << 56 is 0.

  // We have overrun the maximum size of a varint (10 bytes).  The data
  // must be corrupt.
  return -1;

done:
  *value = (static_cast<uint64_t>(part0)) |
    (static_cast<uint64_t>(part1) << 28) |
    (static_cast<uint64_t>(part2) << 56);
  return static_cast<int>(ptr - buffer);
}

inline int WireFormat::ReadSignedVarint32(const uint8_t* buffer, 
  int32_t* value) {
  uint32_t uvalue = 0;
  int res = ReadVarint32(buffer, &uvalue);
  *value = ZigZagDecode32(uvalue);
  return res;
}

inline int WireFormat::ReadSignedVarint64(const uint8_t* buffer, 
  int64_t* value) {
  uint64_t uvalue = 0;
  int res = ReadVarint64(buffer, &uvalue);
  *value = ZigZagDecode64(uvalue);
  return res;
}

} // namespace vss

#endif // VSS_WIRE_FORMAT_INI_H__