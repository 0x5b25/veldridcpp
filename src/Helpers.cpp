
#include "veldrid/Helpers.hpp"

#include <cassert>

namespace Veldrid::Helpers
{
    std::uint32_t Clamp(std::uint32_t value, std::uint32_t min, std::uint32_t max)
    {
        if (value <= min)
        {
            return min;
        }
        else if (value >= max)
        {
            return max;
        }
        else
        {
            return value;
        }
    }
    std::uint32_t GetDimension(std::uint32_t largestLevelDimension, std::uint32_t mipLevel)
    {
        std::uint32_t ret = largestLevelDimension;
        for (std::uint32_t i = 0; i < mipLevel; i++)
        {
            ret /= 2;
        }

        return std::max(1U, ret);
    }
    void GetMipDimensions(
        Veldrid::sp<Veldrid::Texture>& tex, 
        std::uint32_t mipLevel, 
        std::uint32_t& width, std::uint32_t& height, std::uint32_t& depth)
    {
        auto& desc = tex->GetDesc();
        width = GetDimension(desc.width, mipLevel);
        height = GetDimension(desc.height, mipLevel);
        depth = GetDimension(desc.depth, mipLevel);
    }
    
    void GetMipLevelAndArrayLayer(
        sp<Veldrid::Texture>& tex, std::uint32_t subresource, 
        std::uint32_t& mipLevel, std::uint32_t& arrayLayer)
    {
        arrayLayer = subresource / tex->GetDesc().mipLevels;
        mipLevel = subresource - (arrayLayer * tex->GetDesc().mipLevels);
    }

    std::uint64_t ComputeSubresourceOffset(
        sp<Veldrid::Texture>& tex, std::uint32_t mipLevel, std::uint32_t arrayLayer)
    {
        //assert(tex->GetDesc().usage.staging == true);
        return ComputeArrayLayerOffset(tex, arrayLayer) + ComputeMipOffset(tex, mipLevel);
    }

    std::uint32_t ComputeArrayLayerOffset(
        Veldrid::sp<Veldrid::Texture>& tex, std::uint32_t arrayLayer)
    {
        if (arrayLayer == 0)
        {
            return 0;
        }

        std::uint32_t blockSize = FormatHelpers::IsCompressedFormat(tex->GetDesc().format) ? 4u : 1u;
        std::uint32_t layerPitch = 0;
        for (std::uint32_t level = 0; level < tex->GetDesc().mipLevels; level++)
        {
            std::uint32_t mipWidth, mipHeight, mipDepth;
            
            GetMipDimensions(tex, level, mipWidth, mipHeight, mipDepth);
            std::uint32_t storageWidth =  std::max(mipWidth, blockSize);
            std::uint32_t storageHeight = std::max(mipHeight, blockSize);
            layerPitch += FormatHelpers::GetRegionSize(storageWidth, storageHeight, mipDepth, tex->GetDesc().format);
        }

        return layerPitch * arrayLayer;
    }

    std::uint32_t ComputeMipOffset(
        sp<Veldrid::Texture>& tex, std::uint32_t mipLevel)
    {
        std::uint32_t blockSize = FormatHelpers::IsCompressedFormat(tex->GetDesc().format) ? 4u : 1u;
        std::uint32_t offset = 0;
        for (std::uint32_t level = 0; level < mipLevel; level++)
        {
            std::uint32_t mipWidth, mipHeight, mipDepth;
            
            GetMipDimensions(tex, level, mipWidth, mipHeight, mipDepth);
            std::uint32_t storageWidth =  std::max(mipWidth, blockSize);
            std::uint32_t storageHeight = std::max(mipHeight, blockSize);
            offset += FormatHelpers::GetRegionSize(storageWidth, storageHeight, mipDepth, tex->GetDesc().format);
        }

        return offset;
    }

    

    namespace FormatHelpers{

        bool IsFormatViewCompatible(
            const PixelFormat& viewFormat, const PixelFormat& realFormat)
        {
            if (IsCompressedFormat(realFormat))
            {
                return IsSrgbCounterpart(viewFormat, realFormat);
            }
            else
            {
                return GetViewFamilyFormat(viewFormat) == GetViewFamilyFormat(realFormat);
            }
        }

        std::uint32_t GetNumRows(std::uint32_t height, const PixelFormat& format)
        {
            switch (format)
            {
            case PixelFormat::BC1_Rgb_UNorm:
            case PixelFormat::BC1_Rgb_UNorm_SRgb:
            case PixelFormat::BC1_Rgba_UNorm:
            case PixelFormat::BC1_Rgba_UNorm_SRgb:
            case PixelFormat::BC2_UNorm:
            case PixelFormat::BC2_UNorm_SRgb:
            case PixelFormat::BC3_UNorm:
            case PixelFormat::BC3_UNorm_SRgb:
            case PixelFormat::BC4_UNorm:
            case PixelFormat::BC4_SNorm:
            case PixelFormat::BC5_UNorm:
            case PixelFormat::BC5_SNorm:
            case PixelFormat::BC7_UNorm:
            case PixelFormat::BC7_UNorm_SRgb:
            case PixelFormat::ETC2_R8_G8_B8_UNorm:
            case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
            case PixelFormat::ETC2_R8_G8_B8_A8_UNorm:
                return (height + 3) / 4;

            default:
                return height;
            }
        }

        std::uint32_t GetDepthPitch(std::uint32_t rowPitch, std::uint32_t height, PixelFormat format)
        {
            return rowPitch * GetNumRows(height, format);
        }

        std::uint32_t GetRegionSize(std::uint32_t width, std::uint32_t height, std::uint32_t depth, PixelFormat format)
        {
            std::uint32_t blockSizeInBytes;
            if (IsCompressedFormat(format))
            {
                assert((width % 4 == 0 || width < 4) && (height % 4 == 0 || height < 4));
                blockSizeInBytes = GetBlockSizeInBytes(format);
                width /= 4;
                height /= 4;
            }
            else
            {
                blockSizeInBytes = GetSizeInBytes(format);
            }

            return width * height * depth * blockSizeInBytes;
        }

        std::uint32_t GetSizeInBytes(const PixelFormat& format)
        {
            switch (format)
            {
                case PixelFormat::R8_UNorm:
                case PixelFormat::R8_SNorm:
                case PixelFormat::R8_UInt:
                case PixelFormat::R8_SInt:
                    return 1;

                case PixelFormat::R16_UNorm:
                case PixelFormat::R16_SNorm:
                case PixelFormat::R16_UInt:
                case PixelFormat::R16_SInt:
                case PixelFormat::R16_Float:
                case PixelFormat::R8_G8_UNorm:
                case PixelFormat::R8_G8_SNorm:
                case PixelFormat::R8_G8_UInt:
                case PixelFormat::R8_G8_SInt:
                    return 2;

                case PixelFormat::R32_UInt:
                case PixelFormat::R32_SInt:
                case PixelFormat::R32_Float:
                case PixelFormat::R16_G16_UNorm:
                case PixelFormat::R16_G16_SNorm:
                case PixelFormat::R16_G16_UInt:
                case PixelFormat::R16_G16_SInt:
                case PixelFormat::R16_G16_Float:
                case PixelFormat::R8_G8_B8_A8_UNorm:
                case PixelFormat::R8_G8_B8_A8_UNorm_SRgb:
                case PixelFormat::R8_G8_B8_A8_SNorm:
                case PixelFormat::R8_G8_B8_A8_UInt:
                case PixelFormat::R8_G8_B8_A8_SInt:
                case PixelFormat::B8_G8_R8_A8_UNorm:
                case PixelFormat::B8_G8_R8_A8_UNorm_SRgb:
                case PixelFormat::R10_G10_B10_A2_UNorm:
                case PixelFormat::R10_G10_B10_A2_UInt:
                case PixelFormat::R11_G11_B10_Float:
                case PixelFormat::D24_UNorm_S8_UInt:
                    return 4;

                case PixelFormat::D32_Float_S8_UInt:
                    return 5;

                case PixelFormat::R16_G16_B16_A16_UNorm:
                case PixelFormat::R16_G16_B16_A16_SNorm:
                case PixelFormat::R16_G16_B16_A16_UInt:
                case PixelFormat::R16_G16_B16_A16_SInt:
                case PixelFormat::R16_G16_B16_A16_Float:
                case PixelFormat::R32_G32_UInt:
                case PixelFormat::R32_G32_SInt:
                case PixelFormat::R32_G32_Float:
                    return 8;

                case PixelFormat::R32_G32_B32_A32_Float:
                case PixelFormat::R32_G32_B32_A32_UInt:
                case PixelFormat::R32_G32_B32_A32_SInt:
                    return 16;

                case PixelFormat::BC1_Rgb_UNorm:
                case PixelFormat::BC1_Rgb_UNorm_SRgb:
                case PixelFormat::BC1_Rgba_UNorm:
                case PixelFormat::BC1_Rgba_UNorm_SRgb:
                case PixelFormat::BC2_UNorm:
                case PixelFormat::BC2_UNorm_SRgb:
                case PixelFormat::BC3_UNorm:
                case PixelFormat::BC3_UNorm_SRgb:
                case PixelFormat::BC4_UNorm:
                case PixelFormat::BC4_SNorm:
                case PixelFormat::BC5_UNorm:
                case PixelFormat::BC5_SNorm:
                case PixelFormat::BC7_UNorm:
                case PixelFormat::BC7_UNorm_SRgb:
                case PixelFormat::ETC2_R8_G8_B8_UNorm:
                case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
                case PixelFormat::ETC2_R8_G8_B8_A8_UNorm:
                    assert(false);
                    //Debug.Fail("GetSizeInBytes should not be used on a compressed format.");
                default: return 0;
            }
        }

        std::uint32_t GetSizeInBytes(const ShaderDataType& format)
        {
            switch (format)
            {
                case ShaderDataType::Byte2_Norm:
                case ShaderDataType::Byte2:
                case ShaderDataType::SByte2_Norm:
                case ShaderDataType::SByte2:
                case ShaderDataType::Half1:
                    return 2;
                case ShaderDataType::Float1:
                case ShaderDataType::UInt1:
                case ShaderDataType::Int1:
                case ShaderDataType::Byte4_Norm:
                case ShaderDataType::Byte4:
                case ShaderDataType::SByte4_Norm:
                case ShaderDataType::SByte4:
                case ShaderDataType::UShort2_Norm:
                case ShaderDataType::UShort2:
                case ShaderDataType::Short2_Norm:
                case ShaderDataType::Short2:
                case ShaderDataType::Half2:
                    return 4;
                case ShaderDataType::Float2:
                case ShaderDataType::UInt2:
                case ShaderDataType::Int2:
                case ShaderDataType::UShort4_Norm:
                case ShaderDataType::UShort4:
                case ShaderDataType::Short4_Norm:
                case ShaderDataType::Short4:
                case ShaderDataType::Half4:
                    return 8;
                case ShaderDataType::Float3:
                case ShaderDataType::UInt3:
                case ShaderDataType::Int3:
                    return 12;
                case ShaderDataType::Float4:
                case ShaderDataType::UInt4:
                case ShaderDataType::Int4:
                    return 16;
                default: return 0;
            }
        }

        std::int32_t GetElementCount(ShaderDataType format)
        {
            switch (format)
            {
                case ShaderDataType::Float1:
                case ShaderDataType::UInt1:
                case ShaderDataType::Int1:
                case ShaderDataType::Half1:
                    return 1;
                case ShaderDataType::Float2:
                case ShaderDataType::Byte2_Norm:
                case ShaderDataType::Byte2:
                case ShaderDataType::SByte2_Norm:
                case ShaderDataType::SByte2:
                case ShaderDataType::UShort2_Norm:
                case ShaderDataType::UShort2:
                case ShaderDataType::Short2_Norm:
                case ShaderDataType::Short2:
                case ShaderDataType::UInt2:
                case ShaderDataType::Int2:
                case ShaderDataType::Half2:
                    return 2;
                case ShaderDataType::Float3:
                case ShaderDataType::UInt3:
                case ShaderDataType::Int3:
                    return 3;
                case ShaderDataType::Float4:
                case ShaderDataType::Byte4_Norm:
                case ShaderDataType::Byte4:
                case ShaderDataType::SByte4_Norm:
                case ShaderDataType::SByte4:
                case ShaderDataType::UShort4_Norm:
                case ShaderDataType::UShort4:
                case ShaderDataType::Short4_Norm:
                case ShaderDataType::Short4:
                case ShaderDataType::UInt4:
                case ShaderDataType::Int4:
                case ShaderDataType::Half4:
                    return 4;
                default: return 0;
            }
        }

        std::uint32_t GetSampleCountUInt32(Veldrid::Texture::Description::SampleCount sampleCount){
            switch (sampleCount)
            {
                case Veldrid::Texture::Description::SampleCount::x1:
                    return 1;
                case Veldrid::Texture::Description::SampleCount::x2:
                    return 2;
                case Veldrid::Texture::Description::SampleCount::x4:
                    return 4;
                case Veldrid::Texture::Description::SampleCount::x8:
                    return 8;
                case Veldrid::Texture::Description::SampleCount::x16:
                    return 16;
                case Veldrid::Texture::Description::SampleCount::x32:
                    return 32;
                default:return 0;
            }
        }

        bool IsStencilFormat(PixelFormat format)
        {
            return format
                == PixelFormat::D24_UNorm_S8_UInt
                || format == PixelFormat::D32_Float_S8_UInt;
        }

        bool IsDepthStencilFormat(PixelFormat format)
        {
            return format == PixelFormat::D32_Float_S8_UInt
                || format == PixelFormat::D24_UNorm_S8_UInt
                || format == PixelFormat::R16_UNorm
                || format == PixelFormat::R32_Float;
        }

        bool IsCompressedFormat(PixelFormat format)
        {
            return format == PixelFormat::BC1_Rgb_UNorm
                || format == PixelFormat::BC1_Rgb_UNorm_SRgb
                || format == PixelFormat::BC1_Rgba_UNorm
                || format == PixelFormat::BC1_Rgba_UNorm_SRgb
                || format == PixelFormat::BC2_UNorm
                || format == PixelFormat::BC2_UNorm_SRgb
                || format == PixelFormat::BC3_UNorm
                || format == PixelFormat::BC3_UNorm_SRgb
                || format == PixelFormat::BC4_UNorm
                || format == PixelFormat::BC4_SNorm
                || format == PixelFormat::BC5_UNorm
                || format == PixelFormat::BC5_SNorm
                || format == PixelFormat::BC7_UNorm
                || format == PixelFormat::BC7_UNorm_SRgb
                || format == PixelFormat::ETC2_R8_G8_B8_UNorm
                || format == PixelFormat::ETC2_R8_G8_B8_A1_UNorm
                || format == PixelFormat::ETC2_R8_G8_B8_A8_UNorm;
        }

        std::uint32_t GetRowPitch(std::uint32_t width, const PixelFormat& format)
        {
            switch (format)
            {
                case PixelFormat::BC1_Rgb_UNorm:
                case PixelFormat::BC1_Rgb_UNorm_SRgb:
                case PixelFormat::BC1_Rgba_UNorm:
                case PixelFormat::BC1_Rgba_UNorm_SRgb:
                case PixelFormat::BC2_UNorm:
                case PixelFormat::BC2_UNorm_SRgb:
                case PixelFormat::BC3_UNorm:
                case PixelFormat::BC3_UNorm_SRgb:
                case PixelFormat::BC4_UNorm:
                case PixelFormat::BC4_SNorm:
                case PixelFormat::BC5_UNorm:
                case PixelFormat::BC5_SNorm:
                case PixelFormat::BC7_UNorm:
                case PixelFormat::BC7_UNorm_SRgb:
                case PixelFormat::ETC2_R8_G8_B8_UNorm:
                case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
                case PixelFormat::ETC2_R8_G8_B8_A8_UNorm: {
                    auto blocksPerRow = (width + 3) / 4;
                    auto blockSizeInBytes = GetBlockSizeInBytes(format);
                    return blocksPerRow * blockSizeInBytes;
                }
                default: {
                    return width * GetSizeInBytes(format);
                }
            }
        }

        std::uint32_t GetBlockSizeInBytes(const PixelFormat& format)
        {
            switch (format)
            {
                case PixelFormat::BC1_Rgb_UNorm:
                case PixelFormat::BC1_Rgb_UNorm_SRgb:
                case PixelFormat::BC1_Rgba_UNorm:
                case PixelFormat::BC1_Rgba_UNorm_SRgb:
                case PixelFormat::BC4_UNorm:
                case PixelFormat::BC4_SNorm:
                case PixelFormat::ETC2_R8_G8_B8_UNorm:
                case PixelFormat::ETC2_R8_G8_B8_A1_UNorm:
                    return 8;
                case PixelFormat::BC2_UNorm:
                case PixelFormat::BC2_UNorm_SRgb:
                case PixelFormat::BC3_UNorm:
                case PixelFormat::BC3_UNorm_SRgb:
                case PixelFormat::BC5_UNorm:
                case PixelFormat::BC5_SNorm:
                case PixelFormat::BC7_UNorm:
                case PixelFormat::BC7_UNorm_SRgb:
                case PixelFormat::ETC2_R8_G8_B8_A8_UNorm:
                    return 16;
                default:return 0;
            }
        }

        bool IsSrgbCounterpart(const PixelFormat& viewFormat, const PixelFormat& realFormat)
        {
            //NotImplementedException!!
            return viewFormat == realFormat;
        }

        Veldrid::Texture::Description::SampleCount GetSampleCount(std::uint32_t samples)
        {
            switch (samples)
            {
                case 1: return Veldrid::Texture::Description::SampleCount::x1;
                case 2: return Veldrid::Texture::Description::SampleCount::x2;
                case 4: return Veldrid::Texture::Description::SampleCount::x4;
                case 8: return Veldrid::Texture::Description::SampleCount::x8;
                case 16: return Veldrid::Texture::Description::SampleCount::x16;
                case 32: return Veldrid::Texture::Description::SampleCount::x32;
                default: return Veldrid::Texture::Description::SampleCount::x1;
            }
        }

        PixelFormat GetViewFamilyFormat(const PixelFormat& format) {
            switch (format) {
                case PixelFormat::R32_G32_B32_A32_Float:
                case PixelFormat::R32_G32_B32_A32_UInt:
                case PixelFormat::R32_G32_B32_A32_SInt:
                    return PixelFormat::R32_G32_B32_A32_Float;
                case PixelFormat::R16_G16_B16_A16_Float:
                case PixelFormat::R16_G16_B16_A16_UNorm:
                case PixelFormat::R16_G16_B16_A16_UInt:
                case PixelFormat::R16_G16_B16_A16_SNorm:
                case PixelFormat::R16_G16_B16_A16_SInt:
                    return PixelFormat::R16_G16_B16_A16_Float;
                case PixelFormat::R32_G32_Float:
                case PixelFormat::R32_G32_UInt:
                case PixelFormat::R32_G32_SInt:
                    return PixelFormat::R32_G32_Float;
                case PixelFormat::R10_G10_B10_A2_UNorm:
                case PixelFormat::R10_G10_B10_A2_UInt:
                    return PixelFormat::R10_G10_B10_A2_UNorm;
                case PixelFormat::R8_G8_B8_A8_UNorm:
                case PixelFormat::R8_G8_B8_A8_UNorm_SRgb:
                case PixelFormat::R8_G8_B8_A8_UInt:
                case PixelFormat::R8_G8_B8_A8_SNorm:
                case PixelFormat::R8_G8_B8_A8_SInt:
                    return PixelFormat::R8_G8_B8_A8_UNorm;
                case PixelFormat::R16_G16_Float:
                case PixelFormat::R16_G16_UNorm:
                case PixelFormat::R16_G16_UInt:
                case PixelFormat::R16_G16_SNorm:
                case PixelFormat::R16_G16_SInt:
                    return PixelFormat::R16_G16_Float;
                case PixelFormat::R32_Float:
                case PixelFormat::R32_UInt:
                case PixelFormat::R32_SInt:
                    return PixelFormat::R32_Float;
                case PixelFormat::R8_G8_UNorm:
                case PixelFormat::R8_G8_UInt:
                case PixelFormat::R8_G8_SNorm:
                case PixelFormat::R8_G8_SInt:
                    return PixelFormat::R8_G8_UNorm;
                case PixelFormat::R16_Float:
                case PixelFormat::R16_UNorm:
                case PixelFormat::R16_UInt:
                case PixelFormat::R16_SNorm:
                case PixelFormat::R16_SInt:
                    return PixelFormat::R16_Float;
                case PixelFormat::R8_UNorm:
                case PixelFormat::R8_UInt:
                case PixelFormat::R8_SNorm:
                case PixelFormat::R8_SInt:
                    return PixelFormat::R8_UNorm;
                case PixelFormat::BC1_Rgba_UNorm:
                case PixelFormat::BC1_Rgba_UNorm_SRgb:
                case PixelFormat::BC1_Rgb_UNorm:
                case PixelFormat::BC1_Rgb_UNorm_SRgb:
                    return PixelFormat::BC1_Rgba_UNorm;
                case PixelFormat::BC2_UNorm:
                case PixelFormat::BC2_UNorm_SRgb:
                    return PixelFormat::BC2_UNorm;
                case PixelFormat::BC3_UNorm:
                case PixelFormat::BC3_UNorm_SRgb:
                    return PixelFormat::BC3_UNorm;
                case PixelFormat::BC4_UNorm:
                case PixelFormat::BC4_SNorm:
                    return PixelFormat::BC4_UNorm;
                case PixelFormat::BC5_UNorm:
                case PixelFormat::BC5_SNorm:
                    return PixelFormat::BC5_UNorm;
                case PixelFormat::B8_G8_R8_A8_UNorm:
                case PixelFormat::B8_G8_R8_A8_UNorm_SRgb:
                    return PixelFormat::B8_G8_R8_A8_UNorm;
                case PixelFormat::BC7_UNorm:
                case PixelFormat::BC7_UNorm_SRgb:
                    return PixelFormat::BC7_UNorm;
                default:
                    return format;
            }
        }

    }
} // namespace Veldrid::Helpers

