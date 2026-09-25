#include "../../include/filehandling/imageloader.h"

ImageLoader::ImageLoader()
{
}

ImageLoader::ImageLoader(const std::string &filePath)
{
    this->m_filePath = filePath;
    m_imagestream.open(m_filePath, std::ios_base::in | std::ios_base::binary);
    this->ReadImageContents();
}

ImageLoader::~ImageLoader()
{
    m_imagestream.close();
}

unsigned long ImageLoader::GetImageSize() const
{
    return m_imageSize;
}

byte* ImageLoader::ImageContents()
{
    return this->m_imageContents.get();
}

void ImageLoader::ReadImageContents()
{
    if (!m_imagestream.is_open())
    {
        return;
    }

    m_imagestream.seekg(0, std::ios_base::end);
    const std::streamoff fileSizeBytes = m_imagestream.tellg();
    m_imagestream.seekg(0, std::ios_base::beg);

    // tellg()/seekg() report failure via a negative offset (e.g. std::streamoff(-1))
    // rather than an exception, so this must be checked explicitly: casting a
    // negative value straight to an unsigned size previously produced a huge
    // allocation request.
    if (!m_imagestream.good() || fileSizeBytes <= 0)
    {
        return;
    }

    const auto imageSize = static_cast<std::size_t>(fileSizeBytes);
    auto buffer = std::make_unique<byte[]>(imageSize);
    m_imagestream.read(reinterpret_cast<char*>(buffer.get()), static_cast<std::streamsize>(imageSize));

    // Only expose as many bytes as were actually read. A short/truncated read
    // must never be reported as a full-size image, since callers (e.g.
    // IOComponent) size their own buffers directly off GetImageSize().
    const auto bytesRead = static_cast<unsigned long>(m_imagestream.gcount());
    m_imageContents = std::move(buffer);
    m_imageSize = bytesRead;
}
