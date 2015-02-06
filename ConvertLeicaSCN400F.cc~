////////////////////////////////////////////////////////////////////////////////////////
// C++ Code to Convert Leica Whole Slide Fluorescence Images to Binary Files 
// For Leica Model SCN400F scanner only.
//
// History:
//   2015-Feb-05: M.Freed Created
//
//
// To Compile (on linux):
//   g++ -Wall -L/usr/lib64 -ltiff -o ConvertLeicaSCN400F ConvertLeicaSCN400F.cc
//
//   Note: libtiff 4 or higher must be installed on your computer
//         Replace /usr/lib64 with the location of libtiff 4 libraries on your computer
//
//
// To Run (on linux):
// ./ConvertLeicaSCN400F filename_input filename_output_prefix
//
//
// Example:
// ./ConvertLeicaSCN400F Slide_899633L_DAPI_CD31_COLIV.scn data_converted/Slide_899633L_DAPI_CD31_COLIV_
//
//
// Output:
//   The program will generate a series of files, one for each channel of each field, 
//     where a field is a single sample on the slide. 
//
//   Output filenames = filename_output_prefix+'ImageA_ChannelB_XCCCC_YDDDDD.bin'
//                      A = An integer starting at 0 that specifies the field
//                      B = 0: Red
//                          1: Green
//                          2: Blue
//                      CCCC = The number of pixels in the X dimension
//                      DDDDD = The number of pixels in the Y dimension
//                      File format = Binary, Unsigned 8 Bit Integer
//
//
// Notes about reading highest resolution pixel data from Leica fluorescence images:
// [Information from Benjamin Gilbert @ OpenSlide]
//    -Leica .scn files are in BigTIFF format
//    -Read the IMAGEDESCRIPTION TIF TAG to get an XML description of the file
//    -Look for the <image> (or <image>s) with a <view> that doesn't match
//     the <collection> dimensions. 
//    -For the highest resolution level, read the TIFF directories specified in
//     the "ifd" attribute for the <dimension>s which have an "r" of 0
//    -"c" refers to a <channel> in <channelSettings>
////////////////////////////////////////////////////////////////////////////////////////

extern "C" {
  #include <tiffio.h>
}
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <ctime>
using namespace std;

int main (int argc, char * argv[])
{

  // Read Inputs
  string fn_in, fn_outprefix;
  if (argc != 3) return -1;
  else
  {
    fn_in=argv[1];
    fn_outprefix=argv[2];
  }

  // Open File
  TIFF *tif=TIFFOpen(fn_in.c_str(), "r");

  // Get Image Description In First Directory
  int iTIFFdir=0;
  char *sdescription=0;
  TIFFGetField(tif,TIFFTAG_IMAGEDESCRIPTION,&sdescription);
  //cout << sdescription << endl; 

  // Read Through XML Data
  // Get TIFF Directories You Want
  int istart=0,iend=0;
  char *chold;
  //int icount=0;
  long xcollection=0,ycollection=0; bool flag_foundcollection=false;
  int ifind=0,ifind1=0,ifind2=0;
  int carray[3]={-1,-1,-1}, darray[3]={-1,-1,-1}, iarray[3]={-1,-1,-1}, cvalue, dvalue, ivalue=0;
  vector<int> channelID, TIFFDirectories, ImageNo;
  string shold,shold1,shold2;
  while (sdescription[istart] != '\0') // && icount < 100000)
  {
    // Get Size of Next Line
    while (sdescription[iend] != '\n' && sdescription[iend] != '\0' && sdescription[iend] != '\r') {iend++;}

    if (iend != istart) 
    {
      // Allocate Character Array With Enough Space  
      chold=new char[iend-istart+1];

      // Copy Data To Character Array
      for (int ii=istart;ii<iend;ii++) {chold[ii-istart]=sdescription[ii];}
      chold[iend-istart]='\0';

      // Convert to a String
      shold=chold;

      // Look for <collection> Dimensions
      if (!flag_foundcollection)
      {
        ifind=shold.find("<collection"); 
        if (ifind != -1) 
        {
          ifind1=shold.find("sizeX=\"")+7;
          ifind2=shold.find("\"",ifind1+1);
          shold1=shold.substr(ifind1,ifind2-ifind1);
          xcollection=atol(shold1.c_str());
          ifind1=shold.find("sizeY=\"")+7;
          ifind2=shold.find("\"",ifind1+1);
          shold1=shold.substr(ifind1,ifind2-ifind1);
          ycollection=atol(shold1.c_str());
          flag_foundcollection=true;
        }
      }
      else
      {
        //For Each <image> 
        if (shold.find("<dimension ") != string::npos && shold.find(" r=\"0\"") != string::npos && shold.find(" c=") != string::npos)
        {
          // Save c and ifd Attributes
          ifind1=shold.find("c=\"")+3;
          ifind2=shold.find("\"",ifind1+1);
          shold1=shold.substr(ifind1,ifind2-ifind1);
          cvalue=atoi(shold1.c_str());
          ifind1=shold.find("ifd=\"")+5;
          ifind2=shold.find("\"",ifind1+1);
          shold1=shold.substr(ifind1,ifind2-ifind1);
          dvalue=atoi(shold1.c_str());

          if (cvalue == 0) {carray[0]=cvalue; darray[0]=dvalue; iarray[0]=ivalue;}
          if (cvalue == 1) {carray[1]=cvalue; darray[1]=dvalue; iarray[1]=ivalue;}
          if (cvalue == 2) {carray[2]=cvalue; darray[2]=dvalue; iarray[2]=ivalue;}
        }

        // If <view> Size Doesn't Match Collection Size then Save Directories
        if (shold.find("<view ") != string::npos)
        {
          ifind1=shold.find("sizeX=\"")+7;
          ifind2=shold.find("\"",ifind1+1);
          shold1=shold.substr(ifind1,ifind2-ifind1);

          ifind1=shold.find("sizeY=\"")+7;
          ifind2=shold.find("\"",ifind1+1);
          string shold2=shold.substr(ifind1,ifind2-ifind1);

          if (atol(shold1.c_str()) != xcollection && atol(shold2.c_str()) != ycollection)
          {
            if (carray[0] != -1) channelID.push_back(carray[0]);
            if (carray[1] != -1) channelID.push_back(carray[1]);
            if (carray[2] != -1) channelID.push_back(carray[2]);
            if (darray[0] != -1) TIFFDirectories.push_back(darray[0]);
            if (darray[1] != -1) TIFFDirectories.push_back(darray[1]);
            if (darray[2] != -1) TIFFDirectories.push_back(darray[2]);
            if (iarray[0] != -1) ImageNo.push_back(iarray[0]);
            if (iarray[1] != -1) ImageNo.push_back(iarray[1]);
            if (iarray[2] != -1) ImageNo.push_back(iarray[2]);
            ivalue++;
          } 
        }
      }

      // Free Memory
      delete [] chold;

      // Initialize For Next Line
      while (sdescription[iend] == '\n' || sdescription[iend] == '\r') {iend++;}
      istart=iend;
      iend=istart;

    }
    //icount++;
  }

  // Read Through TIFF Directories To Get The Ones You Want
  uint32 ww,hh;
  bool flag_dir=false;
  int iwrite;
  while (TIFFReadDirectory(tif))
  {
    iTIFFdir++;
    for (uint32 ii=0;ii<TIFFDirectories.size();ii++) 
    {
      if (TIFFDirectories[ii]==iTIFFdir)
      {
        iwrite=ii;
        flag_dir=true;
      }
    }
    if (flag_dir)
    {
      // Get Image Size
      TIFFGetField(tif,TIFFTAG_IMAGEWIDTH,&ww);
      TIFFGetField(tif,TIFFTAG_IMAGELENGTH,&hh);

      // Create Output Filename
      ostringstream convert;
      convert << fn_outprefix << "Image" << ImageNo[iwrite] << "_Channel" << channelID[iwrite] << "_X" << ww << "_Y" << hh << ".bin"; 
      string fn_out=convert.str(); 

      // Read In Image Data
      uint32 Npixels=ww*hh;
      uint32* raster;
      raster=(uint32*) _TIFFmalloc(Npixels*sizeof(uint32));
      uint8* image=new uint8[Npixels];
      if (raster != NULL)
      {
        if (TIFFReadRGBAImage(tif,ww,hh,raster,0)) 
        {
          cout << "Read: Successful (" << ww << " x " << hh << ")" << endl;
          for (uint32 ii=0;ii<Npixels;ii++)
          {
            switch (channelID[iwrite])
            {
              case 0: image[ii]=static_cast<uint8>(TIFFGetR(raster[ii])); break;
              case 1: image[ii]=static_cast<uint8>(TIFFGetG(raster[ii])); break;
              case 2: image[ii]=static_cast<uint8>(TIFFGetB(raster[ii])); break;
              default: cout << "Invalid channelID" << endl;
            }
          }
        } 
        else {cout << "Read: Unsuccessful" << endl;}
      }
      else {cout << "Memory Allocation: Unsuccessful" << endl;}
      _TIFFfree(raster);

      // Write Out Image Data In Binary Format
      cout << "Writing " << fn_out << endl << endl;
      ofstream ofile;
      ofile.open(fn_out.c_str(), ios::out | ios::binary);
      ofile.write((char *) image, sizeof(uint8)*Npixels); 
      ofile.close();

      // Free Memory
      delete [] image;

    }
    flag_dir=false;
  }

  // Close TIFF File
  TIFFClose(tif);


  return 0;
}



