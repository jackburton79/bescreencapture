#ifndef __MOVIE_ENCODER_H
#define __MOVIE_ENCODER_H

#include <MediaDefs.h>
#include <MediaFile.h>
#include <Path.h>

#include <queue>

class BBitmap;
class MovieEncoder {
public:
	MovieEncoder();
	~MovieEncoder();
	
	void DisposeData();
		 
	status_t SetSource(BList *fileList, const bool ownsList = true);
	status_t SetCursorQueue(std::queue<BPoint> *queue);
	
	status_t SetOutputFile(const char *fileName);
	status_t SetDestFrame(const BRect &rect);
	
	void SetColorSpace(const color_space &space);
	
	status_t SetQuality(const float &quality);
	
	status_t SetThreadPriority(const int32 &value);
	status_t SetMessenger(const BMessenger &messenger);
	
	media_file_format	MediaFileFormat() const;
	media_format_family MediaFormatFamily() const;
	media_format		MediaFormat() const;
	media_codec_info	MediaCodecInfo() const;
	
	void SetMediaFileFormat(const media_file_format&);
	void SetMediaFormatFamily(const media_format_family &);
	void SetMediaFormat(const media_format &);
	void SetMediaCodecInfo(const media_codec_info &);

	status_t Encode(const media_format_family &, const media_file_format& fileFormat,
			const media_format &, const media_codec_info &,
			const color_space &space = B_RGB32);
	status_t Encode();
	
private:
	void ResetConfiguration();
	
	BBitmap *GetCursorBitmap(const uint8 *data);
	status_t PopCursorPosition(BPoint &point);
			
	int32 fPriority;
	BMessenger fMessenger;
		
	BList *fFileList;
	std::queue<BPoint> *fCursorQueue;
	
	BPath fOutputFile;
	
	BRect fDestFrame;
	color_space fColorSpace;

	media_file_format	fFileFormat;
	media_format_family	fFamily;
	media_format		fFormat;
	media_codec_info	fCodecInfo;
};


#endif
