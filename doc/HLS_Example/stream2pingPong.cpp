/*******************************************************************************
Vendor: Xilinx
Purpose: Programmable Logic based DMA with AXI stream (Zynq HP) & AXI full (Zynq GP) interfaces
Functionality:  The design has two buffers - ping & pong (thanks to the HLS DATAFLOW command).  
				Each buffer collects data until it reaches the BLOCK_SIZE (size found in stream2pingpong.h) 
				& begins data transfer.  AXI data transfer rate can be measured by monitoring the 
				axi*_dataTransferMarker pin.  CPU interrupts (axi*_interrupt) are enabled when a 
				data transfer is started.  CPU Interrupts are cleared via the axi*_cpuRelease GPIOs
Device: All
Revision History: January 16, 2014 - rev 1.0, Mike M.

*******************************************************************************
Copyright (C) 2014 XILINX, Inc.

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials
distributed herewith. Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law:
(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX
HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising under
or in connection with these materials, including for any direct, or any indirect,
special, incidental, or consequential loss or damage (including loss of data,
profits, goodwill, or any type of loss or damage suffered as a result of any
action brought by a third party) even if such damage or loss was reasonably
foreseeable or Xilinx had been advised of the possibility of the same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring fail-safe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer asresultes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES.

*******************************************************************************/


#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#ifndef __SYNTHESIS__
// include path for ap_int & hls_stream for MS Visual C++
#include "C:\EDA\Xilinx\Vivado_HLS\2013.4\include\ap_int.h"
#include "C:\EDA\Xilinx\Vivado_HLS\2013.4\include\hls_stream.h"
#else
#include "ap_int.h"
#include "hls_stream.h"
#endif
#include "stream2pingPong.h"
using namespace std;

#ifdef AXI_MASTER
void stream2pingPong( // I/O type
		hls::stream<uint32_t> &str_in, // AXI Stream PL DMA input							input
		bool *axi_interrupt, // axi stream interrupt to signal data transfer is starting		output
		bool axi_cpuRelease, // CPU GPIO clears an axi stream interrupt						input
		unsigned *pingPongBuffer, // AXI Master PL DMA output									input
		ap_uint<1> aximl_dataTransferMarker, // marks start & stop of axi lite data transfer			output
		unsigned int m_axi_dst_offset
		) // used for axi lite memory offset address				input
#else
void stream2pingPong( // I/O type
		hls::stream<uint32_t> &str_out, // AXI Stream PL DMA output							output
		hls::stream<uint32_t> &str_in, // AXI Stream PL DMA input							input
		bool *axi_interrupt, // axi stream interrupt to signal data transfer is starting		output
		bool axi_cpuRelease, // CPU GPIO clears an axi stream interrupt						input
		ap_uint<1> axis_dataTransferMarker // marks start & stop of axi stream data transfer			output
		) // used for axi lite memory offset address				input
#endif
{
	#pragma HLS INTERFACE ap_ctrl_hs port=return

	// Define the 'str_in' interface as an AXI4 stream slave
	#pragma HLS interface ap_fifo port=str_in
	#pragma HLS resource core=AXI4Stream variable=str_in metadata="-bus_bundle AXI4Stream_M" port_map={str_in TDATA}

	// HLS does not support ap_none interface signals for RTL verification testing
	// uncomment the following GPIO signals when using C synthesis design to build hardware
	// because the below signals are GPIO signals from/to the processor
	#pragma HLS interface ap_none port=axi_cpuRelease
	#pragma HLS interface ap_none port=axi_interrupt
#ifdef AXI_MASTER
	#pragma HLS interface ap_none port=aximl_dataTransferMarker
#else
	#pragma HLS interface ap_none port=axis_dataTransferMarker
#endif

#ifdef AXI_MASTER
	// Define the 'pingPongBuffer' interface as an AXI4 master
	#pragma HLS INTERFACE ap_bus depth=1024 port=pingPongBuffer
	#pragma HLS RESOURCE variable=pingPongBuffer core=AXI4M
	// Define the axi lite slave interface offset for CPU addressing
	#pragma HLS RESOURCE variable=m_axi_dst_offset core=AXI4LiteS metadata="-bus_bundle ap_ctrl_hs_if"
#else
	// Define the 'str_out' interface as an AXI4 stream master
	#pragma HLS interface ap_fifo port=str_out
	#pragma HLS RESOURCE variable=str_out core=AXI4Stream port_map={str_out TDATA} metadata="-bus_bundle AXI4Stream_M"
#endif

//	static ap_uint<10> pingPongPtr = 0; // for C synthesis this assumes max is 10 bits (for 1024 deep buffer) to allow rollover instead of using a reset
//	static ap_uint<16> tmpStat; // retain values between function calls

	// sysmem is required so we can change size of DMA transfer on the fly
	static ap_uint<32> sysmem[BLOCK_SIZE]; 

#pragma HLS DATAFLOW // dataflow establishes concurrency to occur for axi_stream & axim_lite interfaces

	// fetch sample and write to internal storage buffer - sysmem
		axis_read : for (ap_uint<16> tlread = 0; tlread < BLOCK_SIZE; tlread++) {
			#pragma HLS PIPELINE
			sysmem[tlread] = str_in.read();
		}


#ifdef AXI_MASTER
		// AXI-Master Lite interface write
		axim_lite : for (ap_uint<16> tlpong = 0; tlpong < BLOCK_SIZE; tlpong++) {
			#pragma HLS PIPELINE
			*(pingPongBuffer+tlpong+m_axi_dst_offset) = sysmem[tlpong]; // axi master write
				if (tlpong == 0 || tlpong == 1023)
					aximl_dataTransferMarker = aximl_dataTransferMarker+1; // 1 == xfer has started; 0 == xfer has stopped

			if (tlpong == BLOCK_SIZE-1) *axi_interrupt = 1;
			else if (axi_cpuRelease == 1) *axi_interrupt = 0;
		}
#else
		// // AXI-Stream interface write
		axi_stream : for (ap_uint<16> tlping = 0; tlping < BLOCK_SIZE; tlping++) { // assumes 1K points
			#pragma HLS PIPELINE
			str_out << sysmem[tlping]; // axi stream write
			if (tlping == 0 || tlping == 1023)
				axis_dataTransferMarker = axis_dataTransferMarker+1; // 1 == xfer has started; 0 == xfer has stopped

			if (tlping == BLOCK_SIZE-1) *axi_interrupt = 1;
			else if (axi_cpuRelease == 1) *axi_interrupt = 0;

		}
#endif
}