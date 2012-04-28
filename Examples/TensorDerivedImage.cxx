/*=========================================================================

  Program:   ITK General
  Module:    $RCSfile: TensorDerivedImage.cxx,v $
  Language:  C++
  Date:      $Date: 2009/03/17 18:58:59 $
  Version:   $Revision: 1.2 $

  Author: Jeffrey T. Duda (jtduda@seas.upenn.edu)
  Institution: PICSL

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

=========================================================================*/

#include "antsUtilities.h"
#include <algorithm>

#include <stdio.h>

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
// #include "itkDiffusionTensor3D.h"
// #include "itkTensorFractionalAnisotropyImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include <string>
#include "TensorFunctions.h"
#include "ReadWriteImage.h"
#include "itkRGBPixel.h"

namespace ants
{
// entry point for the library; parameter 'args' is equivalent to 'argv' in (argc,argv) of commandline parameters to
// 'main()'
int TensorDerivedImage( std::vector<std::string> args, std::ostream* out_stream = NULL )
{
  // put the arguments coming in as 'args' into standard (argc,argv) format;
  // 'args' doesn't have the command name as first, argument, so add it manually;
  // 'args' may have adjacent arguments concatenated into one argument,
  // which the parser should handle
  args.insert( args.begin(), "TensorDerivedImage" );

  std::remove( args.begin(), args.end(), std::string( "" ) );
  int     argc = args.size();
  char* * argv = new char *[args.size() + 1];
  for( unsigned int i = 0; i < args.size(); ++i )
    {
    // allocate space for the string plus a null character
    argv[i] = new char[args[i].length() + 1];
    std::strncpy( argv[i], args[i].c_str(), args[i].length() );
    // place the null character in the end
    argv[i][args[i].length()] = '\0';
    }
  argv[argc] = 0;
  // class to automatically cleanup argv upon destruction
  class Cleanup_argv
  {
public:
    Cleanup_argv( char* * argv_, int argc_plus_one_ ) : argv( argv_ ), argc_plus_one( argc_plus_one_ )
    {
    }

    ~Cleanup_argv()
    {
      for( unsigned int i = 0; i < argc_plus_one; ++i )
        {
        delete[] argv[i];
        }
      delete[] argv;
    }

private:
    char* *      argv;
    unsigned int argc_plus_one;
  };
  Cleanup_argv cleanup_argv( argv, argc + 1 );

  antscout->set_stream( out_stream );

  // Pixel and Image typedefs
  typedef float                                 PixelType;
  typedef itk::Image<PixelType, 3>              ScalarImageType;
  typedef itk::ImageFileWriter<ScalarImageType> WriterType;

  // typedef itk::Vector<PixelType, 6>             TensorType;
  typedef itk::SymmetricSecondRankTensor<float, 3> TensorType;
  typedef itk::Image<TensorType, 3>                TensorImageType;
  typedef itk::ImageFileReader<TensorImageType>    ReaderType;
  typedef itk::RGBPixel<float>                     ColorPixelType;
  typedef itk::Image<ColorPixelType, 3>            ColorImageType;
  typedef itk::ImageFileWriter<ColorImageType>     ColorWriterType;

  // Check for valid input paramters
  if( argc < 3 )
    {
    antscout << "Usage: " << argv[0] << " tensorvolume outputvolume outputtype" << std::endl;
    return 1;
    }

  // Input parameters
  char *      inputName = argv[1];
  char *      outputName = argv[2];
  std::string outType = argv[3];

  TensorImageType::Pointer dtimg;
  ReadTensorImage<TensorImageType>(dtimg, inputName, false);

  antscout << "tensor_image: " << inputName << std::endl;
  antscout << "output_image: " << outputName << std::endl;

  ScalarImageType::Pointer outImage;
  ColorImageType::Pointer  colorImage;

  if( outType == "DEC" )
    {
    colorImage = ColorImageType::New();
    colorImage->SetRegions(dtimg->GetLargestPossibleRegion() );
    colorImage->SetSpacing(dtimg->GetSpacing() );
    colorImage->SetOrigin(dtimg->GetOrigin() );
    colorImage->SetDirection(dtimg->GetDirection() );
    colorImage->Allocate();
    }
  else
    {
    outImage = ScalarImageType::New();
    outImage->SetRegions(dtimg->GetLargestPossibleRegion() );
    outImage->SetSpacing(dtimg->GetSpacing() );
    outImage->SetOrigin(dtimg->GetOrigin() );
    outImage->SetDirection(dtimg->GetDirection() );
    outImage->Allocate();
    }

  itk::ImageRegionIteratorWithIndex<TensorImageType> inputIt(dtimg,
                                                             dtimg->GetLargestPossibleRegion() );

  if( (outType == "XX") || (outType == "xx") )
    {
    outType = "0";
    }
  if( (outType == "XY") || (outType == "YX") || (outType == "xy") || (outType == "yx") )
    {
    outType = "1";
    }
  if( (outType == "XZ") || (outType == "ZX") || (outType == "xz") || (outType == "zx") )
    {
    outType = "2";
    }
  if( (outType == "YY") || (outType == "yy") )
    {
    outType = "3";
    }
  if( (outType == "YZ") || (outType == "ZY") || (outType == "yz") || (outType == "zy") )
    {
    outType = "4";
    }
  if( (outType == "ZZ") || (outType == "zz") )
    {
    outType = "5";
    }

  antscout << "Calculating output..." << std::flush;

  while( !inputIt.IsAtEnd() )
    {
    if( (outType == "0") || (outType == "1") || (outType == "2") || (outType == "3") ||
        (outType == "4") || (outType == "5") )
      {
      int idx = atoi(outType.c_str() );
      outImage->SetPixel(inputIt.GetIndex(), inputIt.Value()[idx]);
      }
    else if( (outType == "TR") || (outType == "MD") )
      {
      ScalarImageType::PixelType tr;
      tr = inputIt.Value()[0] + inputIt.Value()[2] + inputIt.Value()[5];
      if( tr < 0 )
        {
        tr = 0;
        }
      if( tr != tr )
        {
        tr = 0;
        }
      if( outType == "TR" )
        {
        outImage->SetPixel(inputIt.GetIndex(), tr);
        }
      else
        {
        outImage->SetPixel(inputIt.GetIndex(), tr / 3.0);
        }
      }
    else if( outType == "V" )
      {
      // unsigned int invalids = 0;
      bool       success = true;
      TensorType t = TensorLogAndExp<TensorType>(inputIt.Value(), true, success);
      int        current = 1;
      if( !success )
        {
        current = 0;
        }
      outImage->SetPixel(inputIt.GetIndex(), current);
      // antscout << "Found " << invalids << " invalid tensors" << std::endl;
      }
    else if( outType == "FA" )
      {
      float fa = GetTensorFA<TensorType>(inputIt.Value() );
      outImage->SetPixel(inputIt.GetIndex(), fa);
      }
    else if( outType == "DEC" )
      {
      colorImage->SetPixel(inputIt.GetIndex(),
                           GetTensorRGB<TensorType>(inputIt.Value() ) );
      ++inputIt;
      }
    }

  antscout << "Done. " << std::endl;

  if( outType == "DEC" )
    {
    antscout << "output origin: " << colorImage->GetOrigin() << std::endl;
    antscout << "output size: " << colorImage->GetLargestPossibleRegion().GetSize() << std::endl;
    antscout << "output spacing: " << colorImage->GetSpacing() << std::endl;
    antscout << "output direction: " << colorImage->GetDirection() << std::endl;
    }
  else
    {
    antscout << "output origin: " << outImage->GetOrigin() << std::endl;
    antscout << "output size: " << outImage->GetLargestPossibleRegion().GetSize() << std::endl;
    antscout << "output spacing: " << outImage->GetSpacing() << std::endl;
    antscout << "output direction: " << outImage->GetDirection() << std::endl;
    }

  if( outType == "DEC" )
    {
    ColorWriterType::Pointer writer = ColorWriterType::New();
    writer->SetInput( colorImage );
    writer->SetFileName( outputName );
    writer->Update();
    }
  else
    {
    WriterType::Pointer writer = WriterType::New();
    writer->SetInput( outImage );
    writer->SetFileName( outputName );
    writer->Update();
    }

  return 0;
}
} // namespace ants
