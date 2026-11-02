#include "FileStreamWriter.h"

namespace HBL2
{
    FileStreamWriter::FileStreamWriter(const std::filesystem::path& path)
        : m_Path(path)
    {
        m_Stream = std::ofstream(path, std::ofstream::out | std::ofstream::binary);
    }

    FileStreamWriter::~FileStreamWriter()
    {
        m_Stream.close();
    }
    
    bool FileStreamWriter::IsStreamGood() const
    {
        return m_Stream.good();
    }
    
    uint64_t FileStreamWriter::GetStreamPosition()
    {
        return m_Stream.tellp();
    }

    void FileStreamWriter::SetStreamPosition(uint64_t position)
    {
        m_Stream.seekp(position);
    }

    bool FileStreamWriter::WriteData(const char* data, size_t size)
    {
        m_Stream.write(data, size);
        return true;
    }

    void FileStreamWriter::WriteZero(uint64_t size)
    {
        char zero = 0;
        for (uint64_t i = 0; i < size; i++)
        {
            WriteData(&zero, 1);
        }
    }
}