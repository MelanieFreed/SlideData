////////////////////////////////////////////////////////////////////////////////////////
// C++ Code to Convert Leica Whole Slide Fluorescence Images to Binary Files 
// For Leica Model SCN400F scanner only.
//
// History:
//   2015-Feb-05: M.Freed Created
//
//
// To Compile (on linux):
//   g++ -Wall -L/usr/lib64 -ltiff -lxml2 -o ConvertLeicaSCN400F ConvertLeicaSCN400F.cc
//
//   Note: libtiff 4 or higher and libxml2 must be installed on your computer
//         Replace /usr/lib64 with the location of libtiff 4 and libxml2 libraries on your computer
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
//   Exit Codes:
//     0: Success
//     1: Could not open Leica .scn file
//     2: Could not parse XML description in .scn file
//     3: Could not read image from Leica .scn file
//     4: Could not allocate memory for image
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

////////////////////////////////////////////////////////////////////////////////////////
// The MIT License (MIT)
//
// Copyright (c) 2015 Melanie Freed
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////////////

extern "C" {
  #include <tiffio.h>
  #include <libxml/tree.h>
  #include <libxml/parser.h>
  #include <libxml/xpath.h>
}
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <ctime>
using namespace std;

// Error Codes
// Exit Code: 0 = Success
void Error_TIFFOpen(void)
{
  cout << "ERROR (ConvertLeicaSCN400F.cc): Could Not Open Leica .SCN File." << endl;
} // Exit Code: 1
void Error_XMLParse(void)
{
  cout << "ERROR (ConvertLeicaSCN400F.cc): Could Not Parse XML Description." << endl;
} // Exit Code: 2
void Error_ImageRead(void)
{
  cout << "ERROR (ConvertLeicaSCN400F.cc): Could Not Read Image From .SCN File." << endl;
} // Exit Code: 3
void Error_MemoryAllocate(void)
{
  cout << "ERROR (ConvertLeicaSCN400F.cc): Could Not Allocate Memory For Image." << endl;
} // Exit Code: 4

int main (int argc, char * argv[])
{

  //////////////////////////////////////////////////////////////////////////////////////
  // Read Inputs
  //////////////////////////////////////////////////////////////////////////////////////
  string fn_in, fn_outprefix;
  if (argc != 3) return -1;
  else
  {
    fn_in=argv[1];
    fn_outprefix=argv[2];
  }


  //////////////////////////////////////////////////////////////////////////////////////
  // Open .scn File
  //////////////////////////////////////////////////////////////////////////////////////
  TIFF *tif=TIFFOpen(fn_in.c_str(), "r");
  if (tif == NULL) {atexit(Error_TIFFOpen); exit(1);}


  //////////////////////////////////////////////////////////////////////////////////////
  // Get Image Description In First Directory of .scn File
  //////////////////////////////////////////////////////////////////////////////////////
  int iTIFFdir=0;
  char *sdescription=0;
  TIFFGetField(tif,TIFFTAG_IMAGEDESCRIPTION,&sdescription);
  //cout << sdescription << endl; 


  //////////////////////////////////////////////////////////////////////////////////////
  // Process XML Data
  // Figure Out Which TIFF Directories You Want
  //////////////////////////////////////////////////////////////////////////////////////

  // Replace <scn ...> with <scn> to avoid using Leica's namespace
  bool flag_stop=false;
  int jj=0;
  while (!flag_stop)
  {
    if (sdescription[jj]=='<' && sdescription[jj+1]=='s' && 
        sdescription[jj+2]=='c' && sdescription[jj+3]=='n')
    { 
      jj=jj+4;
      while (sdescription[jj] != '>') {sdescription[jj]=' '; jj++;} 
      flag_stop=true;
    }
    jj++;
  }

  // Parse the XML
  int xsize=0,xi=0;
  while (sdescription[xi] != '\0') {xsize++; xi++;}
  xmlInitParser();
  LIBXML_TEST_VERSION
  xmlDocPtr xmldoc;
  xmlXPathContextPtr xmlcontext;
  xmlXPathObjectPtr xmlresult;
  xmlChar *keyword;
  xmldoc=xmlParseMemory(sdescription,xsize);
  if (xmldoc == NULL) {atexit(Error_XMLParse); exit(2);}
  xmlcontext=xmlXPathNewContext(xmldoc);

  // Get Collection Dimensions
  long xcollection=0,ycollection=0; 
  xmlresult=xmlXPathEvalExpression((xmlChar *) "//collection",xmlcontext);
  keyword=xmlGetProp(xmlresult->nodesetval->nodeTab[0],(xmlChar *) "sizeX");
  xcollection=atol((char *) keyword);
  xmlFree(keyword);
  keyword=xmlGetProp(xmlresult->nodesetval->nodeTab[0],(xmlChar *) "sizeY");
  ycollection=atol((char *) keyword);
  xmlFree(keyword);
  xmlXPathFreeObject(xmlresult);

  // Save Information For All Images You Want
  vector<int> channelID, TIFFDirectories, ImageNo;
  long xview=0,yview=0;
  ostringstream convert; string ssearch;
  int icount=0;
  jj=1; convert << "//image[" << jj << "]/view"; ssearch=convert.str();
  xmlresult=xmlXPathEvalExpression((xmlChar *) ssearch.c_str(),xmlcontext);
  while  (!xmlXPathNodeSetIsEmpty(xmlresult->nodesetval))
  {
    // Compare <view> Dimensions With <collection> Dimensions
    keyword=xmlGetProp(xmlresult->nodesetval->nodeTab[0],(xmlChar *) "sizeX");
    xview=atol((char *) keyword);
    xmlFree(keyword);
    keyword=xmlGetProp(xmlresult->nodesetval->nodeTab[0],(xmlChar *) "sizeY");
    yview=atol((char *) keyword);
    xmlFree(keyword);
    if (xview != xcollection && yview != ycollection)
    {
      // Save Information For <view>s Whose Dimensions Do Not Match <collection>
      xmlXPathFreeObject(xmlresult);
      convert.str(""); convert.clear(); convert << "//image[" << jj << "]/pixels/dimension[@r=0]"; ssearch=convert.str(); 
      xmlresult=xmlXPathEvalExpression((xmlChar *) ssearch.c_str(),xmlcontext);
      for (int ii=0;ii<xmlresult->nodesetval->nodeNr;ii++)
      {
        keyword=xmlGetProp(xmlresult->nodesetval->nodeTab[ii],(xmlChar *) "c");
        channelID.push_back(atoi((char*) keyword));
        xmlFree(keyword);
        keyword=xmlGetProp(xmlresult->nodesetval->nodeTab[ii],(xmlChar *) "ifd");
        TIFFDirectories.push_back(atoi((char*) keyword));
        xmlFree(keyword);
        ImageNo.push_back(icount);
      }
      icount++;
    }

    xmlXPathFreeObject(xmlresult);
    jj++; convert.str(""); convert.clear(); convert << "//image[" << jj << "]/view"; ssearch=convert.str(); 
    xmlresult=xmlXPathEvalExpression((xmlChar *) ssearch.c_str(),xmlcontext);
  }

  // Clean Up
  xmlXPathFreeContext(xmlcontext);
  xmlFreeDoc(xmldoc);
  xmlCleanupParser();


  //////////////////////////////////////////////////////////////////////////////////////
  // Get Data From .scn File
  //////////////////////////////////////////////////////////////////////////////////////
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
        else {atexit(Error_ImageRead); exit(3);}
      }
      else {atexit(Error_MemoryAllocate); exit(4);}
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



