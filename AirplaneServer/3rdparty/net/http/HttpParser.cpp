#include "http_parser.h"

using namespace std;

#include "HttpParser.h"

HTTPParser::HTTPParser(http_parser_type parserType)
{
    mTmpHeadStr = nullptr;
    mTmpHeadLen = 0;

    mIsWebSocket = false;
    mIsKeepAlive = false;
    mISCompleted = false;
    mParserType = parserType;
    mSettings.on_status = sStatusHandle;
    mSettings.on_body = sBodyHandle;
    mSettings.on_url = sUrlHandle;
    mSettings.on_header_value = sHeadValue;
    mSettings.on_header_field = sHeadField;
    mSettings.on_headers_complete = sHeadComplete;
    mSettings.on_message_begin = sMessageBegin;
    mSettings.on_message_complete = sMessageEnd;
    mSettings.on_chunk_header = sChunkHeader;
    mSettings.on_chunk_complete = sChunkComplete;
    mParser.data = this;
    // ��ʼ��������
    http_parser_init(&mParser, mParserType);
}

void HTTPParser::clearParse()
{
    mISCompleted = false;
    mHeadValues.clear();
    mPath.clear();
    mQuery.clear();
}

bool HTTPParser::isWebSocket() const
{
    return mIsWebSocket;
}

bool HTTPParser::isKeepAlive() const
{
    return mIsKeepAlive;
}

bool HTTPParser::checkCompleted(const char* buffer, int len)
{
    const static char* RL = "\r\n";
    const static int RL_LEN = strlen(RL);

    const static char* DOUBLE_RL = "\r\n\r\n";
    const static int DOUBLE_RL_LEN = strlen(DOUBLE_RL);

    const static char* CONTENT_LENGTH_FLAG = "Content-Length: ";
    const static int CONTENT_LENGTH_FLAG_LEN = strlen(CONTENT_LENGTH_FLAG);

    const static char* CHUNKED_FLAG = "Transfer-Encoding: chunked";
    const static int CHUNKED_FLAG_LEN = strlen(CHUNKED_FLAG);

    std::string copyBuffer(buffer, len);
    copyBuffer.push_back(0);

    const char* headlineend = strstr(copyBuffer.c_str(), DOUBLE_RL);
    if (headlineend == nullptr) return false;

    const char* bodystart = headlineend + DOUBLE_RL_LEN;

    const char* contentlen_find = strstr(copyBuffer.c_str(), CONTENT_LENGTH_FLAG);
    if (contentlen_find != nullptr)
    {
        char temp[1024];
        int num = 0;
        const char* content_len_flag_start = contentlen_find + CONTENT_LENGTH_FLAG_LEN;
        const char* content_len_flag_end = strstr(content_len_flag_start, "\r\n");

        for (; content_len_flag_start < content_len_flag_end; ++content_len_flag_start)
        {
            temp[num++] = *content_len_flag_start;
        }
        temp[num++] = 0;
        if (num == 1)
        {
            return false;
        }

        const int datalen = atoi(temp);
        if ((len - (bodystart - copyBuffer.c_str())) >= datalen)
        {
            return true;
        }
    }
    else
    {
        const char* has_chunked = strstr(copyBuffer.c_str(), CHUNKED_FLAG);
        if (has_chunked != nullptr)
        {
            bool checkChunked = false;

            const char* tmp = bodystart;
            const char* len_flag = strstr(bodystart, RL);
            while (len_flag != nullptr)
            {
                string numstr(tmp, len_flag);
                int    nValude = 0;
                sscanf(numstr.c_str(), "%x", &nValude);

                /*����Len�ֶκ��RL*/
                len_flag += (RL_LEN);
                tmp = len_flag;
                if (tmp >= (copyBuffer.c_str() + len))
                {
                    break;
                }

                if (nValude > 0)
                {
                    /*��������*/
                    len_flag += nValude;
                    tmp = len_flag;
                    if (tmp >= (copyBuffer.c_str() + len))
                    {
                        break;
                    }

                    /*��������RL*/
                    len_flag = strstr(tmp, RL);
                    if (len_flag != nullptr)
                    {
                        len_flag += RL_LEN;
                        tmp = len_flag;
                    }
                    else
                    {
                        break;
                    }
                }

                if (nValude == 0)
                {
                    checkChunked = true;
                    break;
                }
                else
                {
                    /*ָ����ܴ��ڵ���һ��datalenĩβ��rl*/
                    len_flag = strstr(tmp, RL);
                }
            }

            if (checkChunked)
            {
                /*����Ƿ����Ϲ�����*/
                if (*tmp == '\r')
                {
                    const char* finish = strstr(tmp, RL);
                    return finish != nullptr;
                }
                else
                {
                    const char* finish = strstr(tmp, DOUBLE_RL);
                    return finish != nullptr;
                }
            }
        }
        else
        {
            return true;
        }
    }

    return false;
}

int HTTPParser::tryParse(const char* buffer, int len)
{
    if (!mISCompleted && checkCompleted(buffer, len))
    {
        mISCompleted = true;

        int nparsed = http_parser_execute(&mParser, &mSettings, buffer, len);
        http_parser_init(&mParser, mParserType);

        mIsWebSocket = getValue("Upgrade") == "websocket";
        mIsKeepAlive = !getValue("Keep-Alive").empty();
        mTmpHeadStr = nullptr;
        mTmpHeadLen = 0;
        return len;
    }
    else
    {
        return 0;
    }
}

const std::string& HTTPParser::getPath() const
{
    return mPath;
}

const std::string& HTTPParser::getQuery() const
{
    return mQuery;
}

bool HTTPParser::isCompleted() const
{
    return mISCompleted;
}

std::string HTTPParser::getValue(const std::string& key) const
{
    auto it = mHeadValues.find(key);
    if (it != mHeadValues.end())
    {
        return (*it).second;
    }
    else
    {
        return "";
    }
}

const std::string& HTTPParser::getBody() const
{
    return mBody;
}

int HTTPParser::sChunkHeader(http_parser* hp)
{
    return 0;
}
int HTTPParser::sChunkComplete(http_parser* hp)
{
    return 0;
}
int HTTPParser::sMessageBegin(http_parser* hp)
{
    return 0;
}
int HTTPParser::sMessageEnd(http_parser* hp)
{
    return 0;
}
int HTTPParser::sHeadComplete(http_parser* hp)
{
    return 0;
}

int HTTPParser::sUrlHandle(http_parser* hp, const char *url, size_t length)
{
    struct http_parser_url u;
    HTTPParser* httpParser = (HTTPParser*)hp->data;

    int result = http_parser_parse_url(url, length, 0, &u);
    if (result) {
        return -1;
    }
    else {
        if ((u.field_set & (1 << UF_PATH))) {
            httpParser->mPath = std::string(url + u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);
        }
        else {
            fprintf(stderr, "\n\n*** failed to parse PATH in URL %s ***\n\n", url);
            return -1;
        }

        if ((u.field_set & (1 << UF_QUERY))) {
            httpParser->mQuery = std::string(url + u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);
        }
    }

    return 0;
}

int HTTPParser::sHeadValue(http_parser* hp, const char *at, size_t length)
{
    HTTPParser* httpParser = (HTTPParser*)hp->data;
    httpParser->mHeadValues[string(httpParser->mTmpHeadStr, httpParser->mTmpHeadLen)] = string(at, length);
    return 0;
}

int HTTPParser::sHeadField(http_parser* hp, const char *at, size_t length)
{
    HTTPParser* httpParser = (HTTPParser*)hp->data;
    httpParser->mTmpHeadStr = at;
    httpParser->mTmpHeadLen = length;
    return 0;
}

int HTTPParser::sStatusHandle(http_parser* hp, const char *at, size_t length)
{
    HTTPParser* httpParser = (HTTPParser*)hp->data;
    httpParser->mStatus = string(at, length);
    return 0;
}
int HTTPParser::sBodyHandle(http_parser* hp, const char *at, size_t length)
{
    HTTPParser* httpParser = (HTTPParser*)hp->data;
    httpParser->mBody = string(at, length);
    return 0;
}