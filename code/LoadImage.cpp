#include <wincodec.h>
#include <stdio.h>
#include <memory>


namespace HyperP::Imaging
{
    struct DataLinkedListNode;
    class DataArray;
    struct RawGuide;
    bool GetTopLevelJPEGRawsFromMPOFileName(const char *filename,DataArray *pDataArray);
} // namespace HyperP::Imaging

bool HyperP::Imaging::GetTopLevelJPEGRawsFromMPOFileName(const char *filename,DataArray *pDataArray)
{
    FILE *filedesc;
    {
        errno_t err_open=fopen_s(&filedesc,filename,"r");
        if (err_open!=0)
        {
            return false;
        }
    }
}

struct HyperP::Imaging::DataLinkedListNode
{
    std::unique_ptr<char[]> raw;
    std::unique_ptr<DataLinkedListNode> next;
};

class HyperP::Imaging::DataArray
{
    std::unique_ptr<char> _raw;
    std::unique_ptr<RawGuide[]> _guide;
    size_t _count;
    public:
    bool WhereIs(size_t id,RawGuide *pGuide)
    {
        if (0<=id && id<_count)
        {
            *pGuide=_guide[id];
            return true;
        }
        else
        {
            return false;
        }
    }
};

struct HyperP::Imaging::RawGuide
{
    size_t size;
    char *data;
};
