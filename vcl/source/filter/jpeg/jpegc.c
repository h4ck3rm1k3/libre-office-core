/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <jerror.h>

#include <rtl/alloc.h>
#include <osl/diagnose.h>

#include "transupp.h"
#include "jpeg.h"

struct ErrorManagerStruct
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

void jpeg_svstream_src (j_decompress_ptr cinfo, void* infile);
void jpeg_svstream_dest (j_compress_ptr cinfo, void* outfile);

METHODDEF( void ) errorExit (j_common_ptr cinfo)
{
    ErrorManagerPointer error = (ErrorManagerPointer) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(error->setjmp_buffer, 1);
}

METHODDEF( void ) outputMessage (j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);
}

/* TODO: when incompatible changes are possible again
   the preview size hint should be redone */
static int nPreviewWidth = 0;
static int nPreviewHeight = 0;
void SetJpegPreviewSizeHint( int nWidth, int nHeight )
{
    nPreviewWidth = nWidth;
    nPreviewHeight = nHeight;
}

void ReadJPEG( void* pJPEGReader, void* pInputStream, long* pLines )
{
    struct jpeg_decompress_struct   cinfo;
    struct ErrorManagerStruct       jerr;
    struct JPEGCreateBitmapParam    aCreateBitmapParam;
    HPBYTE                          pDIB;
    HPBYTE                          pTmp;
    long                            nWidth;
    long                            nHeight;
    long                            nAlignedWidth;
    JSAMPLE*                        aRangeLimit;
    HPBYTE                          pScanLineBuffer = NULL;
    long                            nScanLineBufferComponents = 0;

    if ( setjmp( jerr.setjmp_buffer ) )
    {
        jpeg_destroy_decompress( &cinfo );
        return;
    }

    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = errorExit;
    jerr.pub.output_message = outputMessage;

    jpeg_create_decompress( &cinfo );
    jpeg_svstream_src( &cinfo, pInputStream );
    jpeg_read_header( &cinfo, TRUE );

    cinfo.scale_num = 1;
    cinfo.scale_denom = 1;
    cinfo.output_gamma = 1.0;
    cinfo.raw_data_out = FALSE;
    cinfo.quantize_colors = FALSE;
    if ( cinfo.jpeg_color_space == JCS_YCbCr )
        cinfo.out_color_space = JCS_RGB;
    else if ( cinfo.jpeg_color_space == JCS_YCCK )
        cinfo.out_color_space = JCS_CMYK;

    OSL_ASSERT(cinfo.out_color_space == JCS_CMYK || cinfo.out_color_space == JCS_GRAYSCALE || cinfo.out_color_space == JCS_RGB);

    /* change scale for preview import */
    if( nPreviewWidth || nPreviewHeight )
    {
        if( nPreviewWidth == 0 )
        {
            nPreviewWidth = ( cinfo.image_width * nPreviewHeight ) / cinfo.image_height;
            if( nPreviewWidth <= 0 )
            {
                nPreviewWidth = 1;
            }
        }
        else if( nPreviewHeight == 0 )
        {
            nPreviewHeight = ( cinfo.image_height * nPreviewWidth ) / cinfo.image_width;
            if( nPreviewHeight <= 0 )
            {
                nPreviewHeight = 1;
            }
        }

        for( cinfo.scale_denom = 1; cinfo.scale_denom < 8; cinfo.scale_denom *= 2 )
        {
            if( cinfo.image_width < nPreviewWidth * cinfo.scale_denom )
                break;
            if( cinfo.image_height < nPreviewHeight * cinfo.scale_denom )
                break;
        }

        if( cinfo.scale_denom > 1 )
        {
            cinfo.dct_method            = JDCT_FASTEST;
            cinfo.do_fancy_upsampling   = FALSE;
            cinfo.do_block_smoothing    = FALSE;
        }
    }

    jpeg_start_decompress( &cinfo );

    nWidth = cinfo.output_width;
    nHeight = cinfo.output_height;
    aCreateBitmapParam.nWidth = nWidth;
    aCreateBitmapParam.nHeight = nHeight;

    aCreateBitmapParam.density_unit = cinfo.density_unit;
    aCreateBitmapParam.X_density = cinfo.X_density;
    aCreateBitmapParam.Y_density = cinfo.Y_density;
    aCreateBitmapParam.bGray = cinfo.output_components == 1;
    pDIB = CreateBitmapFromJPEGReader( pJPEGReader, &aCreateBitmapParam );
    nAlignedWidth = aCreateBitmapParam.nAlignedWidth;
    aRangeLimit = cinfo.sample_range_limit;

    if ( cinfo.out_color_space == JCS_CMYK )
    {
        nScanLineBufferComponents = cinfo.output_width * 4;
        pScanLineBuffer = rtl_allocateMemory( nScanLineBufferComponents );
    }

    if( pDIB )
    {
        if( aCreateBitmapParam.bTopDown )
        {
            pTmp = pDIB;
        }
        else
        {
            pTmp = pDIB + ( nHeight - 1 ) * nAlignedWidth;
            nAlignedWidth = -nAlignedWidth;
        }

        for ( *pLines = 0; *pLines < nHeight; (*pLines)++ )
        {
            if (pScanLineBuffer != NULL)
            { // in other words cinfo.out_color_space == JCS_CMYK
                int i;
                int j;
                jpeg_read_scanlines( &cinfo, (JSAMPARRAY) &pScanLineBuffer, 1 );
                // convert CMYK to RGB
                for( i=0, j=0; i < nScanLineBufferComponents; i+=4, j+=3 )
                {
                    int color_C = 255 - pScanLineBuffer[i+0];
                    int color_M = 255 - pScanLineBuffer[i+1];
                    int color_Y = 255 - pScanLineBuffer[i+2];
                    int color_K = 255 - pScanLineBuffer[i+3];
                    pTmp[j+0] = aRangeLimit[ 255L - ( color_C + color_K ) ];
                    pTmp[j+1] = aRangeLimit[ 255L - ( color_M + color_K ) ];
                    pTmp[j+2] = aRangeLimit[ 255L - ( color_Y + color_K ) ];
                }
            }
            else
            {
                jpeg_read_scanlines( &cinfo, (JSAMPARRAY) &pTmp, 1 );
            }

            /* PENDING ??? */
            if ( cinfo.err->msg_code == 113 )
                break;

            pTmp += nAlignedWidth;
        }
    }

    if ( pDIB )
    {
        jpeg_finish_decompress( &cinfo );
    }
    else
    {
        jpeg_abort_decompress( &cinfo );
    }

    if (pScanLineBuffer != NULL)
    {
        rtl_freeMemory( pScanLineBuffer );
        pScanLineBuffer = NULL;
    }

    jpeg_destroy_decompress( &cinfo );
}

long WriteJPEG( void* pJPEGWriter, void* pOutputStream,
                long nWidth, long nHeight, long bGreys,
                long nQualityPercent, long aChromaSubsampling,
                void* pCallbackData )
{
    struct jpeg_compress_struct cinfo;
    struct ErrorManagerStruct   jerr;
    void*                       pScanline;
    long                        nY;

    if ( setjmp( jerr.setjmp_buffer ) )
    {
        jpeg_destroy_compress( &cinfo );
        return 0;
    }

    cinfo.err = jpeg_std_error( &jerr.pub );
    jerr.pub.error_exit = errorExit;
    jerr.pub.output_message = outputMessage;

    jpeg_create_compress( &cinfo );
    jpeg_svstream_dest( &cinfo, pOutputStream );

    cinfo.image_width = (JDIMENSION) nWidth;
    cinfo.image_height = (JDIMENSION) nHeight;
    if ( bGreys )
    {
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
    }
    else
    {
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
    }

    jpeg_set_defaults( &cinfo );
    jpeg_set_quality( &cinfo, (int) nQualityPercent, FALSE );

    if ( ( nWidth > 128 ) || ( nHeight > 128 ) )
        jpeg_simple_progression( &cinfo );

    if (aChromaSubsampling == 1) // YUV 4:4:4
    {
        cinfo.comp_info[0].h_samp_factor = 1;
        cinfo.comp_info[0].v_samp_factor = 1;
    }
    else if (aChromaSubsampling == 2) // YUV 4:2:2
    {
        cinfo.comp_info[0].h_samp_factor = 2;
        cinfo.comp_info[0].v_samp_factor = 1;
    }
    else if (aChromaSubsampling == 3) // YUV 4:2:0
    {
        cinfo.comp_info[0].h_samp_factor = 2;
        cinfo.comp_info[0].v_samp_factor = 2;
    }

    jpeg_start_compress( &cinfo, TRUE );

    for( nY = 0; nY < nHeight; nY++ )
    {
        pScanline = GetScanline( pJPEGWriter, nY );

        if( pScanline )
        {
            jpeg_write_scanlines( &cinfo, (JSAMPARRAY) &pScanline, 1 );
        }

        if( JPEGCallback( pCallbackData, nY * 100L / nHeight ) )
        {
            jpeg_destroy_compress( &cinfo );
            return 0;
        }
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress( &cinfo );

    return 1;
}

long Transform(void* pInputStream, void* pOutputStream, long nAngle)
{
    jpeg_transform_info aTransformOption;
    JCOPY_OPTION        aCopyOption = JCOPYOPT_ALL;

    struct jpeg_decompress_struct   aSourceInfo;
    struct jpeg_compress_struct     aDestinationInfo;
    struct ErrorManagerStruct       aSourceError;
    struct ErrorManagerStruct       aDestinationError;

    jvirt_barray_ptr* aSourceCoefArrays      = 0;
    jvirt_barray_ptr* aDestinationCoefArrays = 0;

    aTransformOption.force_grayscale = FALSE;
    aTransformOption.trim            = FALSE;
    aTransformOption.perfect         = FALSE;
    aTransformOption.crop            = FALSE;

    // Angle to transform option
    // 90 Clockwise = 270 Counterclockwise
    switch (nAngle)
    {
        case 2700:
            aTransformOption.transform  = JXFORM_ROT_90;
            break;
        case 1800:
            aTransformOption.transform  = JXFORM_ROT_180;
            break;
        case 900:
            aTransformOption.transform  = JXFORM_ROT_270;
            break;
        default:
            aTransformOption.transform  = JXFORM_NONE;
    }

    // Decompression
    aSourceInfo.err                 = jpeg_std_error(&aSourceError.pub);
    aSourceInfo.err->error_exit     = errorExit;
    aSourceInfo.err->output_message = outputMessage;

    // Compression
    aDestinationInfo.err                 = jpeg_std_error(&aDestinationError.pub);
    aDestinationInfo.err->error_exit     = errorExit;
    aDestinationInfo.err->output_message = outputMessage;

    aDestinationInfo.optimize_coding = TRUE;

    if (setjmp(aSourceError.setjmp_buffer) || setjmp(aDestinationError.setjmp_buffer))
    {
        jpeg_destroy_decompress(&aSourceInfo);
        jpeg_destroy_compress(&aDestinationInfo);
        return 0;
    }

    jpeg_create_decompress(&aSourceInfo);
    jpeg_create_compress(&aDestinationInfo);

    jpeg_svstream_src (&aSourceInfo, pInputStream);

    jcopy_markers_setup(&aSourceInfo, aCopyOption);
    jpeg_read_header(&aSourceInfo, 1);
    jtransform_request_workspace(&aSourceInfo, &aTransformOption);

    aSourceCoefArrays = jpeg_read_coefficients(&aSourceInfo);
    jpeg_copy_critical_parameters(&aSourceInfo, &aDestinationInfo);

    aDestinationCoefArrays = jtransform_adjust_parameters(&aSourceInfo, &aDestinationInfo, aSourceCoefArrays, &aTransformOption);
    jpeg_svstream_dest (&aDestinationInfo, pOutputStream);

    // Compute optimal Huffman coding tables instead of precomuted tables
    aDestinationInfo.optimize_coding = 1;
    jpeg_write_coefficients(&aDestinationInfo, aDestinationCoefArrays);
    jcopy_markers_execute(&aSourceInfo, &aDestinationInfo, aCopyOption);
    jtransform_execute_transformation(&aSourceInfo, &aDestinationInfo, aSourceCoefArrays, &aTransformOption);

    jpeg_finish_compress(&aDestinationInfo);
    jpeg_destroy_compress(&aDestinationInfo);

    jpeg_finish_decompress(&aSourceInfo);
    jpeg_destroy_decompress(&aSourceInfo);

    return 1;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
