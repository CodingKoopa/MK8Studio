#ifndef DDS_H
#define DDS_H

#include <QByteArray>

#include "Common.h"
#include "FormatBase.h"
#include "GX2ImageBase.h"

class DDS : FormatBase
{
public:
  DDS(QByteArray* image_data);

  struct PixelFormat
  {
    quint32 formatSize;
    quint32 pixelFlags;
    QString magic;
    quint32 rgb_bit_count;
    quint32 red_bit_mask;
    quint32 green_bit_mask;
    quint32 blue_bit_mask;
    quint32 alpha_bit_mask;
  };

  struct DDSHeader
  {
    QString magic;
    quint32 header_size;
    quint32 flags;
    quint32 height;
    quint32 width;
    quint32 pitch_or_linear_size;
    quint32 depth;
    quint32 num_mips;
    quint32 reserved1[11];
    PixelFormat pixel_format;
    quint32 complexity_flags;
    quint32 caps2;
    quint32 caps3;
    quint32 caps4;
    quint32 reserved2;
  };

  int MakeHeader(quint32 width, quint32 height, quint32 depth, quint32 num_mips, bool compressed,
                 GX2ImageBase::FormatInfo format_info);

  int WriteFile(QString path);

private:
  enum ImageFlag
  {
    DDS_FLAG_CAPS = 0x1,
    DDS_FLAG_HEIGHT = 0x2,
    DDS_FLAG_WIDTH = 0x4,
    DDS_FLAG_PITCH = 0x8,
    DDS_FLAG_PIXELFORMAT = 0x1000,
    DDS_FLAG_HASMIPMAPS = 0x20000,
    DDS_FLAG_LINEARSIZE = 0x80000,
    DDS_FLAG_DEPTH = 0x800000
  };

  enum ComplexityFlag
  {
    DDS_COMP_FLAG_COMPLEX = 0x8,
    DDS_COMP_FLAG_TEXTURE = 0x1000,
    DDS_COMP_FLAG_HASMIPMAPS = 0x400000
  };

  enum PixelFlag
  {
    DDS_PIX_FLAG_HASALPHA = 0x1,
    DDS_PIX_FLAG_UNCOMPESSEDALPHA = 0x2,
    DDS_PIX_FLAG_COMPRESSED = 0x4,
    DDS_PIX_FLAG_UNCOMPRESSEDRGB = 0x40,
    DDS_PIX_FLAG_UNCOMPRESSEDYUV = 0x200,
    DDS_PIX_FLAG_LUMINANCE = 0x20000,
  };

  QByteArray* m_image_data;
  DDSHeader m_header;
  quint32 m_block_size;
};

#endif  // DDS_H
