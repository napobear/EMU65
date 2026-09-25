#ifndef ROMLOADER_H
#define ROMLOADER_H

#include <fstream>
#include <utility>
#include <memory>
#include "../globals.h"

typedef std::pair<word, byte> data_pair;

/**
 * Loads an AIM-65 6500 assembly binary image from a file.
 */
class ImageLoader
{
 public:
    ImageLoader();
    ImageLoader(const std::string &filePath);
    ~ImageLoader();
    byte* ImageContents();
    /**
     * Number of bytes actually and successfully read from the image file.
     * Zero if the file could not be opened or read. Callers must size any
     * downstream buffer off this value rather than assuming a fixed size.
     */
    unsigned long GetImageSize() const;
 private:
    std::string m_filePath;
    std::ifstream m_imagestream;
    std::unique_ptr<byte[]> m_imageContents; // Array of sequential bytes containing the entire image.
    unsigned long m_imageSize = 0;

    DISALLOW_COPY_AND_ASSIGN(ImageLoader);
    void ReadImageContents();
};

#endif /* ROMLOADER_H */
