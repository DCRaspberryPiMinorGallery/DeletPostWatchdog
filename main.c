#include <stdio.h> 
#include <curl/curl.h> 
#include <string.h> 
#include <stdlib.h> 

struct url_data 
{ 
	size_t size;
	char* data;
};

#define RES_OK  0

unsigned char sParsingHTMLData[128][2048] = { {0x00,}, };
int sParsingDataSize[128] = { 0x00, };

int iFind1stPost = 0;
int iMinIndex = 9999;
int iNowSerchPage = 0;

unsigned char sNickname[512] = "";
unsigned char sIPaddress[16] = "";
unsigned char sPostNumber[32] = "";
unsigned char sPostWriteTime[24] = "";
unsigned char sPostName[1024] = "";
unsigned char sSourceUrl[4096];
unsigned char sUrl[4096];

struct user_post_Data
{
    int iPostNumber;
    char sNickName[512];
    char sPostName[1024];
    char sUrl[4096];
    char sPostWriteTime[24];
};

int fParsingFunction(unsigned char* sReadHTML_Data);
int fStringComparisonFunction(unsigned char* sDataX, unsigned char* sDataY, int iSerchLength);
void fSaveHistoryFuntion(struct user_post_Data postStructData);
int fFileWriteFunction(unsigned char* sFileName, unsigned char* sWriteData, int iWriteSize);
int fExistFindFunction(unsigned char* sFileName);
int fFileLoadFunction(unsigned char* sFileName, unsigned char* sReadBuffer);

size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data) 
{ 
	size_t index = data->size;
	size_t n = (size * nmemb);
	char* tmp;
	data->size += (size * nmemb);
#ifdef DEBUG 
	fprintf(stderr, "data at %p size=%ld nmemb=%ld\n", ptr, size, nmemb);
#endif 
	tmp = realloc(data->data, data->size + 1);
	/* +1 for '\0' */ 
	if(tmp) 
	{ 
		data->data = tmp;
	} 
	else 
	{ 
		if(data->data) 
		{ 
			free(data->data);
		} 
		fprintf(stderr, "Failed to allocate memory.\n");
		return 0;
	} 
	memcpy((data->data + index), ptr, n);
	data->data[data->size] = '\0';
	return size * nmemb;
} 

char *handle_url(char* url) 
{ 
	CURL *curl;
	struct url_data data;
	data.size = 0;
	data.data = malloc(4096);
	/* reasonable size initial buffer */ 
	if(NULL == data.data) 
	{ 
		fprintf(stderr, "Failed to allocate memory.\n");
		return NULL;
	} 
	data.data[0] = '\0';
	CURLcode res;
	curl = curl_easy_init();
	if (curl) 
	{ 
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK) 
		{ 
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		} 
		curl_easy_cleanup(curl);
	} 
	return data.data;
} 

int main(int argc, char* argv[]) 
{ 
    unsigned char* data;
	if(argc < 2) 
	{ 
		fprintf(stderr, "Must provide URL to fetch.\n");
		return 1;
	} 

    int i = 0;
    unsigned char sPage[2048] = "";

#if 0
	data = handle_url(argv[1]);
	if(data) 
	{ 
		//printf("%s\n", data);
        fParsingFunction(data);
		free(data);
	} 
#else
    while (1)
    {
        printf("Update\r\n");

        iFind1stPost = 0;
        iMinIndex = 9999;
        
        for (i = 1; i < 100; i++)
        {
            iNowSerchPage = i;
            if (iFind1stPost == 1)
            {
                i = 100;
            }
            memset(sPage, 0x00, sizeof(sPage));
            sprintf(sPage, "%s&page=%d", argv[1], i);
            data = handle_url(sPage);
            if (data)
            {
                fParsingFunction(data);
                free(data);
            }
            sleep(1);
        }

        printf("done.\r\n");

        //5분마다 실행
        sleep(300);
    }
#endif
	return 0;
}


int fParsingFunction(unsigned char * sReadHTML_Data)
{
    struct user_post_Data postStructData;

    int i = 0;
    int x = 0;
    int y = 0;
    int z = 0;
    
    unsigned int iFindDeletPostIndex_Min = 4294967294;
    unsigned int iFindDeletPostIndex_Max = 0;

    unsigned int sIndexlist[128] = {0x00,};
    int iPostIndex = 0;

    int index = 0;

    int res = strlen(sReadHTML_Data);

    unsigned char sFileAddress[128] = "";

    unsigned char sFileData[2048];

#if 0
    printf("res=%d\r\n", res);

    for (i = 0; i < res; i++)
    {
        printf("%c", sReadHTML_Data[i]);
    }
    printf("\r\n--------------------------------------------------------------------------------------\r\n");
#endif

    for (i = 0; i < res; i++)
    {
        if (fStringComparisonFunction(sReadHTML_Data + i, "<tr class=", 10) == RES_OK)
        {
            if (fStringComparisonFunction(sReadHTML_Data + i + 22, "us-post", 7) == RES_OK)
            {
                for (x = 0; x < res; x++)
                {
                    if (fStringComparisonFunction(sReadHTML_Data + i + x, "</tr>", 5) == RES_OK)
                    {
                        memcpy(sParsingHTMLData[index], sReadHTML_Data + i, x);
                        sParsingDataSize[index] = x;
#if 0
                        printf("sParsingDataSize[%d] = %d\r\n", index, sParsingDataSize[index]);
#endif
                        index++;
                        x = res;
                    }
                }
            }
        }

    }

#if 0
    for (i = 0; i < index; i++)
    {
        printf("[%d]\r\n", i);
        printf("====================================================================================================================================\r\n");
        printf("%s\r\n", sParsingHTMLData[i]);
        printf("====================================================================================================================================\r\n");
    }
#endif

    /*
        글쓴이 (고닉이면 닉네임만, 유동이면 유동닉과 IP 출력)
            고닉  <span class='nickname in' title='10350K'  ><em>10350K</em></span><a class='writer_nikcon'>
            유동  <span class='nickname' title='ㅇㅇ'><em>ㅇㅇ</em></span><span class="ip">(118.235)</span>
        글제목    <a href="/mgallery/board/view/?id=pi&no=702&_rk=EYz&page=2"><em class="icon_img icon_pic"></em>랒파 usb로 바로 부팅 못함?</a>
        글번호    <td class="gall_num">702</td>
        글쓴시각  <td class="gall_date" title="2020-08-03 17:27:51">08.03</td>
    */
    for (i = 0; i < index; i++)
    {
        for (x = 0; x < sParsingDataSize[i]; x++)
        {
            //글번호
            if (fStringComparisonFunction(sParsingHTMLData[i] + x, "<td class=\"gall_num\">", 21) == RES_OK)
            {
                postStructData.iPostNumber = 0;
                memset(postStructData.sNickName, 0x00, sizeof(postStructData.sNickName));
                memset(postStructData.sPostName, 0x00, sizeof(postStructData.sPostName));
                memset(postStructData.sPostWriteTime, 0x00, sizeof(postStructData.sPostWriteTime));
                memset(postStructData.sUrl, 0x00, sizeof(postStructData.sUrl));
#if 0
                printf("---------------------------------------------------------------------------------------------\r\n");
#endif

                for (z = 0; z < sParsingDataSize[i]; z++)
                {
                    if (fStringComparisonFunction(sParsingHTMLData[i] + x + z, "</td>", 5) == RES_OK)
                    {
                        memset(sPostNumber, 0x00, sizeof(sPostNumber));
                        memcpy(sPostNumber, sParsingHTMLData[i] + x + 21, sizeof(char) * z - 21);
                        postStructData.iPostNumber = atoi(sPostNumber);
                        sIndexlist[iPostIndex] = atoi(sPostNumber);
                        iPostIndex++;
#if 0
                        printf("Post Num:%d\r\n", postStructData.iPostNumber);
#endif
                        z = sParsingDataSize[i];
                    }
                }

            }
            
            // URL
            if (fStringComparisonFunction(sParsingHTMLData[i] + x, "<a href=\"", 9) == RES_OK)
            {
                for (z = 0; z < sParsingDataSize[i]; z++)
                {
                    if (fStringComparisonFunction(sParsingHTMLData[i] + x + z, "\">", 2) == RES_OK)
                    {
                        memset(sSourceUrl, 0x00, sizeof(sSourceUrl));
                        memcpy(sSourceUrl, sParsingHTMLData[i] + x + 9, sizeof(char) * z - 9);
                        sprintf(postStructData.sUrl,"https://gall.dcinside.com%s", sSourceUrl);
#if 0
                        printf("URL:%s\r\n", postStructData.sUrl);
#endif
                        z = sParsingDataSize[i];
                    }
                }
            }

            // 글제목
            if (fStringComparisonFunction(sParsingHTMLData[i] + x, "<a href=\"", 9) == RES_OK)
            {
                for (y = 0; y < sParsingDataSize[i]; y++)
                {
                    if (fStringComparisonFunction(sParsingHTMLData[i] + x + y, "</em>", 5) == RES_OK)
                    {
                        for (z = 0; z < sParsingDataSize[i]; z++)
                        {
                            if (fStringComparisonFunction(sParsingHTMLData[i] + x + y + z, "</a>", 4) == RES_OK)
                            {
                                memset(sPostName, 0x00, sizeof(sPostName));
                                memcpy(sPostName, sParsingHTMLData[i] + x + y + 5, sizeof(char) * z - 5);
                                memcpy(postStructData.sPostName, sPostName, sizeof(char) * 1024);
#if 0
                                printf("Post Name:%s\r\n", sPostName);
#endif
                                y = sParsingDataSize[i];
                                z = sParsingDataSize[i];
                            }
                        }
                    }
                }
            }
            
            // 고닉
            if (fStringComparisonFunction(sParsingHTMLData[i] + x, "<span class='nickname in' title='", 33) == RES_OK)
            {
                memset(sNickname, 0x00, sizeof(sNickname));
                for (y = 0; y < sParsingDataSize[i]; y++)
                {
                    if (fStringComparisonFunction(sParsingHTMLData[i] + x + y, "<em>", 4) == RES_OK)
                    {
                        for (z = 0; z < sParsingDataSize[i]; z++)
                        {
                            if (fStringComparisonFunction(sParsingHTMLData[i] + x + y + z, "</em>", 4) == RES_OK)
                            {
                                memcpy(sNickname, sParsingHTMLData[i] + x + y + 4, sizeof(char) * z - 4);
                                memcpy(postStructData.sNickName, sNickname, sizeof(char) * 512);
#if 0
                                printf("고닉:%s\r\n", postStructData.sNickName);
#endif
                                z = sParsingDataSize[i];
                            }
                        }
                    }
                }
            }
            // 유동닉
            if (fStringComparisonFunction(sParsingHTMLData[i] + x, "<span class='nickname' title='", 30) == RES_OK)
            {
                memset(sNickname, 0x00, sizeof(sNickname));
                memset(sIPaddress, 0x00, sizeof(sIPaddress));

                for (y = 0; y < sParsingDataSize[i]; y++)
                {
                    if (fStringComparisonFunction(sParsingHTMLData[i] + x + y, "<em>", 4) == RES_OK)
                    {
                        for (z = 0; z < sParsingDataSize[i]; z++)
                        {
                            if (fStringComparisonFunction(sParsingHTMLData[i] + x + y + z, "</em>", 4) == RES_OK)
                            {
                                memcpy(sNickname, sParsingHTMLData[i] + x + y + 4, sizeof(char) * z - 4);
                                memcpy(postStructData.sNickName, sNickname, sizeof(char) * 512);
                                z = sParsingDataSize[i];
                            }
                        }
                    }
                    if (fStringComparisonFunction(sParsingHTMLData[i] + x + y, "<span class=\"ip\">", 17) == RES_OK)
                    {
                        for (z = 0; z < sParsingDataSize[i]; z++)
                        {
                            if (fStringComparisonFunction(sParsingHTMLData[i] + x + y + z, "</span>", 7) == RES_OK)
                            {
                                memcpy(sIPaddress, sParsingHTMLData[i] + x + y + 17, sizeof(char) * z - 17);
                                sprintf(postStructData.sNickName,"%s%s", postStructData.sNickName, sIPaddress);
#if 0
                                printf("IP유동:%s\r\n", postStructData.sNickName);
#endif
                                z = sParsingDataSize[i];
                            }
                        }
                    }
                }
            }
            
            //글쓴시각
            if (fStringComparisonFunction(sParsingHTMLData[i] + x, "<td class=\"gall_date\" title=\"", 29) == RES_OK)
            {
                for (z = 0; z < sParsingDataSize[i]; z++)
                {
                    if (fStringComparisonFunction(sParsingHTMLData[i] + x + z, "\">", 2) == RES_OK)
                    {
                        memset(sPostWriteTime, 0x00, sizeof(sPostWriteTime));
                        memcpy(sPostWriteTime, sParsingHTMLData[i] + x + 29, sizeof(char) * z - 29);
                        memcpy(postStructData.sPostWriteTime, sPostWriteTime, sizeof(char) * 24);
#if 0
                        printf("Post Time:%s\r\n", postStructData.sPostWriteTime);
#endif
                        z = sParsingDataSize[i];
                    }
                }

                // Data out point
                fSaveHistoryFuntion(postStructData);
            }
        }
    }

    //마지막 페이지 찾기
    for( i = 0 ; i < iPostIndex ; i++)
    {
        //printf("sIndexlist[%d]=%d\r\n", i,sIndexlist[i]);
        if (iFindDeletPostIndex_Min > sIndexlist[i])
        {
            iFindDeletPostIndex_Min = sIndexlist[i];
        }
        if (iFindDeletPostIndex_Max < sIndexlist[i])
        {
            iFindDeletPostIndex_Max = sIndexlist[i];
        }
    }
    if ((iNowSerchPage != 1) && (iFindDeletPostIndex_Max <= 0))
    {
        iFind1stPost = 1;
    }
    printf("[%d]min:%d, max:%d\r\n", iNowSerchPage, iFindDeletPostIndex_Min, iFindDeletPostIndex_Max);

    //삭제글 찾기
    int iDifferenceInValue = 0;
    for (i = 0; i < iPostIndex - 1; i++)
    {
        iDifferenceInValue = sIndexlist[i] - sIndexlist[i + 1];
        if ( iDifferenceInValue > 0 )
        {
            if (iDifferenceInValue != 1)
            {
                if (iDifferenceInValue == 2)
                {
                    printf("Deleted Post detection : %d\r\n", sIndexlist[i] - 1);
                    memset(sFileData,0x00, sizeof(sFileData));
                    memset(sFileAddress, 0x00, sizeof(sFileAddress));
                    sprintf(sFileAddress, "./data/Exist_post/%d", sIndexlist[i] - 1);
                    //printf("./data/Exist_post/%d\r\n", sIndexlist[i] - 1);
                    res = fExistFindFunction(sFileAddress);
                    //printf("res=%d,Index=%d\r\n", res, sIndexlist[i] - 1);
                    if (res)
                    {
                        printf("\r\nFine Local System Backup Info Data\r\n");
                        //sFileData
                        fFileLoadFunction(sFileAddress, sFileData);
                        printf("[Info]\r\n");
                        printf("--------------------------------------------------------------------------------------\r\n");
                        printf("%s\r\n", sFileData);
                        printf("--------------------------------------------------------------------------------------\r\n");
                    }
                }
                else
                {
                    printf("Deleted More Post detection: %d~%d\r\n", sIndexlist[i] - 1, sIndexlist[i + 1] + 1);
                    for (x = sIndexlist[i + 1] + 1; x <= sIndexlist[i] - 1; x++)
                    {
                        memset(sFileAddress, 0x00, sizeof(sFileAddress));
                        sprintf(sFileAddress, "./data/Exist_post/%d", x);
                        //printf("./data/Exist_post/%d\r\n", x);
                        res = fExistFindFunction(sFileAddress);
                        //printf("res=%d,Index=%d\r\n", res,x);
                        if (res)
                        {
                            printf("\r\nFine Local System Backup Info Data\r\n");
                            fFileLoadFunction(sFileAddress, sFileData);
                            printf("[Info] Index : %d\r\n",x);
                            printf("--------------------------------------------------------------------------------------\r\n");
                            printf("%s\r\n", sFileData);
                            printf("--------------------------------------------------------------------------------------\r\n");
                        }
                    }
                }
            }
        }
    }

    return 0;
}

/*
 *	문자열 비교 함수. 같으면 계속 탐색하다가 틀리면 바로 return -1
 */
int fStringComparisonFunction(unsigned char* sDataX, unsigned char* sDataY, int iSerchLength) {
    int x = 0;
    for (x = 0; x < iSerchLength; x++) {
        if (sDataX[x] != sDataY[x]) {
            return -1;
        }
    }
    return 0;
}

void fSaveHistoryFuntion(struct user_post_Data postStructData)
{
    char sWriteBuffer[2048] = "";
    char sFileName[4096] = "";
    int iCount = 0, i;
    int res = 0;
    
    sprintf(sWriteBuffer,"NickName:%s\r\nPostName:%s\r\nURL:%s\r\nPostWriteTime:%s\r\n", postStructData.sNickName, postStructData.sPostName, postStructData.sUrl, postStructData.sPostWriteTime);
    sprintf(sFileName,"./data/Exist_post/%d", postStructData.iPostNumber);

    for ( i = 0 ; i < sizeof(sWriteBuffer) ; i++)
    {
        if (sWriteBuffer[i] == 0x00)
        {
            iCount = i;
            i = sizeof(sWriteBuffer);
        }
    }

    res = fExistFindFunction(sFileName);
    if (res != 1)
    {
        fFileWriteFunction(sFileName, sWriteBuffer, iCount);
    }
    else
    {
#if 0
        printf("이미 존재하는 post 입니다: %d\r\n", postStructData.iPostNumber);
#endif
    }

}


/*
 *	File Write
 */
int fFileWriteFunction(unsigned char* sFileName, unsigned char* sWriteData, int iWriteSize) {
    FILE* fp = fopen(sFileName, "wb");
    if (fp == NULL) {
        printf("File Open Error:%s\r\n", sFileName);
        return -1;
    }
    fwrite(sWriteData, sizeof(char), iWriteSize, fp);
    fclose(fp);
    return 0;
}

/*
 *	Exist File Find.
 */
int fExistFindFunction(unsigned char* sFileName) {
    int res = 1;
    FILE* fp = fopen(sFileName, "rt");
    if (fp == NULL) 
    {
        return 0;
    }
    fclose(fp);
    return res;
}

/*
 *	File Load.
 */
int fFileLoadFunction(unsigned char* sFileName, unsigned char* sReadBuffer) {
    int iConfigFileSize;			//Config File Size 
    FILE* fp = fopen(sFileName, "rt");
    if (fp == NULL) {
        printf("File Open Error:%s\r\n", sFileName);
        return -1;
    }
    fseek(fp, 0L, SEEK_END);		//File Size
    iConfigFileSize = (int)ftell(fp);
    fseek(fp, 0L, SEEK_SET);		//Back to File point [Zero]
    fread(sReadBuffer, sizeof(char), sizeof(char) * iConfigFileSize, fp);
    fclose(fp);
    return iConfigFileSize;
}