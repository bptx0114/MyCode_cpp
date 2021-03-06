// FileClient.cpp: implementation of the CFileClient class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FeiQ.h"
#include "FileClient.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFileClient::CFileClient()
{
    m_ClientSocket = INVALID_SOCKET;
}

CFileClient::~CFileClient()
{
    if( INVALID_SOCKET != m_ClientSocket )
    {
        closesocket( m_ClientSocket );
    }
}

BOOL CFileClient::InitClient( )
{   //创建SOCKET
    m_ClientSocket = socket( AF_INET, 
        SOCK_STREAM, IPPROTO_TCP );
    if( INVALID_SOCKET == m_ClientSocket )
    {
        return FALSE;
    }
    return TRUE;
}

BOOL CFileClient::ConnectServer( LPCSTR pszSvrIP )
{   //连接服务器端
    SOCKADDR_IN addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port   = htons( PORT_FILESVR );
    addr.sin_addr.S_un.S_addr = 
        inet_addr( pszSvrIP );
    if( SOCKET_ERROR == connect( m_ClientSocket, 
        (SOCKADDR *)&addr, sizeof( addr ) ) )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CFileClient::SendFile( LPCTSTR pszFilePath,
                            CProgressCtrl * pwndProgress )
{
    //发送数据头
    NETHEADER header = { 0 };
    header.nVersion  = 1;
    header.nCmdID    = NETCMDID_SENDFILE;
    header.nDataLength = sizeof( SENDFILE );
    SendData( &header, sizeof( header) );
    
    //发送文件信息
    CFile file;
    if( FALSE == file.Open( pszFilePath, 
        CFile::modeRead ) )
    {
        return FALSE;
    }

    SENDFILE sendfile = { 0 };
    strcpy( sendfile.szFileName, file.GetFileName( ) );
    sendfile.nFileLength = file.GetLength( );
    SendData( &sendfile, sizeof( sendfile ) );

    //发送文件数据
    if( pwndProgress != NULL )
    {
        pwndProgress->SetRange32( 0, 
            sendfile.nFileLength );
    }
    CHAR  szBuf[4096] = { 0 };
    DWORD nLeft = file.GetLength( );
    while( nLeft > 0 )
    {   //计算读取的数据量
        DWORD nSend = 4096;
        if( nLeft < nSend )
        {
            nSend = nLeft;
        }
        //读取数据
        file.Read( szBuf, nSend );
        //发送数据
        SendData( szBuf, nSend );
        //计算剩余数据量
        nLeft = nLeft - nSend;
        //更新进度条
        if( pwndProgress != NULL )
        {
            pwndProgress->SetPos( 
                sendfile.nFileLength - nLeft );
        }
    }
    
    //关闭文件
    file.Close( );
    return TRUE;
}

BOOL CFileClient::SendData( LPVOID pData, 
                            DWORD nLen )
{
    LPSTR pTmpData = ( LPSTR )pData;
    int   nLeft    = nLen;
    while( nLeft > 0 )
    {
        int nSend = send( m_ClientSocket, 
            pTmpData, nLeft, 0 );
        if( SOCKET_ERROR == nSend )
        {
            return FALSE;
        }
        nLeft = nLeft - nSend;
        pTmpData = pTmpData + nSend;
    }

    return TRUE;
}
