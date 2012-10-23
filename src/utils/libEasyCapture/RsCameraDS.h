#ifndef ROSOO_DIRECTSHOW_CAMERA_H
#define ROSOO_DIRECTSHOW_CAMERA_H

#define WIN32_LEAN_AND_MEAN

#include <atlbase.h>
#include "qedit.h"
#include "dshow.h"
#include <windows.h>

#include <rscommon.h>

#ifdef _JACKY_USING_OPENCV_
#include <cxcore.h>
#endif

#define MYFREEMEDIATYPE(mt)	{if ((mt).cbFormat != 0)		\
					{CoTaskMemFree((PVOID)(mt).pbFormat);	\
					(mt).cbFormat = 0;						\
					(mt).pbFormat = NULL;					\
				}											\
				if ((mt).pUnk != NULL)						\
				{											\
					(mt).pUnk->Release();					\
					(mt).pUnk = NULL;						\
				}}									

/*
#ifdef LIBEASYCAPTURE_EXPORTS
#	define LIBEASYCAPTURE_API __declspec(dllexport)
#else
#	define LIBEASYCAPTURE_API __declspec(dllimport)
#endif
*/

class CRsCameraDS
{
private:
#ifdef _JACKY_USING_OPENCV_
	IplImage * m_pFrame;
#else
	BYTE*		m_pFrame;
#endif
	BOOL_ m_bConnected;
	int m_nWidth;
	int m_nHeight;
	BOOL_ m_bLock;
	BOOL_ m_bChanged;
	long m_nBufferSize;

	CComPtr<IGraphBuilder> m_pGraph;
	CComPtr<IBaseFilter> m_pDeviceFilter;
	CComPtr<IMediaControl> m_pMediaControl;
	CComPtr<IBaseFilter> m_pSampleGrabberFilter;
	CComPtr<ISampleGrabber> m_pSampleGrabber;
	CComPtr<IPin> m_pGrabberInput;
	CComPtr<IPin> m_pGrabberOutput;
	CComPtr<IPin> m_pCameraOutput;
	CComPtr<IMediaEvent> m_pMediaEvent;
	CComPtr<IBaseFilter> m_pNullFilter;
	CComPtr<IPin> m_pNullInputPin;

private:
	BOOL_ BindFilter(int nCamIDX, IBaseFilter **pFilter);
	void SetCrossBar();

public:
	CRsCameraDS();
	virtual ~CRsCameraDS();

	//打开摄像头，nCamID指定打开哪个摄像头，取值可以为0,1,2,...
	//bDisplayProperties指示是否自动弹出摄像头属性页
	//nWidth和nHeight设置的摄像头的宽和高，如果摄像头不支持所设定的宽度和高度，则返回FALSE_
	BOOL_ CRsCameraDS::OpenCamera(int nCamID, int nWidth, int nHeight, int nFPS, BOOL_ bDisplayProperties);

	//关闭摄像头，析构函数会自动调用这个函数
	void CloseCamera();

	//返回摄像头的数目
	//可以不用创建CRsCameraDS实例，采用int c=CRsCameraDS::CameraCount();得到结果。
	static int CameraCount(); 

	//根据摄像头的编号返回摄像头的名字
	//nCamID: 摄像头编号
	//sName: 用于存放摄像头名字的数组
	//nBufferSize: sName的大小
	//可以不用创建CRsCameraDS实例，采用CRsCameraDS::CameraName();得到结果。
	static int CRsCameraDS::CameraName(int nCamID, char* sName, int nBufferSize);

	//返回图像宽度
	int GetWidth(){return m_nWidth;} 

	//返回图像高度
	int GetHeight(){return m_nHeight;}

	//抓取一帧，返回的IplImage不可手动释放！
	//返回图像数据的为RGB模式的Top-down(第一个字节为左上角像素)，即IplImage::origin=0(IPL_ORIGIN_TL)
#ifdef _JACKY_USING_OPENCV_
	IplImage * QueryFrame();
#else
	BYTE* QueryFrame();
#endif
};

#endif //ROSOO_DIRECTSHOW_CAMERA_H

