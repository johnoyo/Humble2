#include "FileStreamReader.h"

namespace HBL2
{
    FileStreamReader::FileStreamReader(const std::filesystem::path& path)
        : m_Path(path)
    {
        m_Stream = std::ifstream(path, std::ifstream::in | std::ifstream::binary);
    }

    FileStreamReader::~FileStreamReader()
    {
        m_Stream.close();
    }
    
    bool FileStreamReader::IsStreamGood() const
    {
        return m_Stream.good();
    }
    
    uint64_t FileStreamReader::GetStreamPosition()
    {
        return m_Stream.tellg();
    }
    
    void FileStreamReader::SetStreamPosition(uint64_t position)
    {
        m_Stream.seekg(position);
    }

    bool FileStreamReader::ReadData(char* destination, size_t size)
    {
        m_Stream.read(destination, size);
        return true;
    }

    size_t FileStreamReader::GetStreamLength()
    {
        const std::streampos currentPosition = m_Stream.tellg();
        m_Stream.seekg(0, std::ios::end);

        const std::streampos length = m_Stream.tellg() - currentPosition;
        m_Stream.seekg(currentPosition);

        return static_cast<size_t>(length);
    }

    size_t FileStreamReader::ReadAsString(std::string& outString)
    {
        const size_t size = GetStreamLength();

        if (!size)
        {
            return 0;
        }

        outString.resize(size);

        try
        {
            if (m_Stream.read(static_cast<std::ifstream::char_type*> (outString.data()), static_cast<std::streamsize>(size)))
            {
                return size;
            }
            else
            {
                std::string filename = m_Path.filename().string().c_str();
                throw std::runtime_error(std::string("Failed to read from file: " + filename).c_str());
            }

        }
        catch (const std::exception& e)
        {
            std::string filename = m_Path.filename().string().c_str();
            throw std::runtime_error(std::string("Failed to read from file: " + filename + ": " + e.what()).c_str());
        }

        return 0;
    }

    uint32_t FileStreamReader::ReadString(char* dst, uint32_t maxLength)
    {
        uint32_t size = 0;
        ReadRaw(size);

        const uint32_t toRead = std::min(size, maxLength);

        if (toRead > 0)
        {
            ReadData(dst, toRead);
        }

        // String was longer than the destination: skip the rest to stay in sync
        if (size > toRead)
        {
            m_Stream.seekg(static_cast<std::streamoff>(size - toRead), std::ios::cur);
        }

        return toRead;
    }

    void FileStreamReader::ReadString(std::string& out)
    {
        uint32_t size = 0;
        ReadRaw(size);

        // Guard against a corrupt size field allocating gigabytes
        const uint64_t remaining = GetStreamLength();
        if (size > remaining)
        {
            out.clear();
            return;
        }

        out.resize(size);

        if (size > 0)
        {
            ReadData(out.data(), size);
        }
    }

    void FileStreamReader::ReadBytes(ByteSpan& bytes, uint32_t size)
    {
        uint64_t bufferSize = size;
        if (size == UINT32_MAX)
        {
            ReadRaw(bufferSize);
        }

        if (bytes.data() == nullptr)
        {
            bytes.set_data(std::malloc(bufferSize), bufferSize);
        }
        else
        {
            HBL2_CORE_ASSERT(bytes.size() >= bufferSize, "ReadBytes: provided buffer is too small!");
        }

        ReadData((char*)bytes.data(), bytes.size());
    }
}