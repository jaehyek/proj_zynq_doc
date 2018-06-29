#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include "stream2pingPong.h"

#ifndef __SYNTHESIS__
// include path for ap_int & hls_stream for MS Visual C++
#include "C:\EDA\Xilinx\Vivado_HLS\2013.4\include\ap_int.h"
#include "C:\EDA\Xilinx\Vivado_HLS\2013.4\include\hls_stream.h"
#else
#include "ap_int.h"
#include "hls_stream.h"
#endif
using namespace std;

#ifdef AXI_MASTER
void stream2pingPong( // I/O type
		hls::stream<uint32_t> &str_in, // AXI Stream PL DMA input							input
		bool *axi_interrupt, // axi stream interrupt to signal data transfer is starting		output
		bool axi_cpuRelease, // CPU GPIO clears an axi stream interrupt						input
		unsigned *pingPongBuffer, // AXI Master PL DMA output									input
		ap_uint<1> aximl_dataTransferMarker, // marks start & stop of axi lite data transfer			output
		unsigned int m_axi_dst_offset
		); // used for axi lite memory offset address				input
#else
void stream2pingPong( // I/O type
		hls::stream<uint32_t> &str_out, // AXI Stream PL DMA output							output
		hls::stream<uint32_t> &str_in, // AXI Stream PL DMA input							input
		bool *axi_interrupt, // axi stream interrupt to signal data transfer is starting		output
		bool axi_cpuRelease, // CPU GPIO clears an axi stream interrupt						input
		ap_uint<1> axis_dataTransferMarker // marks start & stop of axi stream data transfer			output
		); // used for axi lite memory offset address				input
#endif

int main ()
{
	
unsigned int PF_test = PASS; // test pass / fail test variable
unsigned int sample_cnt;
unsigned int sample_cnt_test, sample_out;
unsigned int block_cnt;

unsigned test_data[BLOCK_SIZE*NUM_TEST_BLOCKS];

unsigned pingPongBuffer[BLOCK_SIZE];
hls::stream<uint32_t> str_in, str_out; // stream in, stream out
ap_uint<32> m_axi_dst_offset = 0; // axi master lite CPU offset address

bool axi_interrupt = 0;
bool axi_cpuRelease = 0;
ap_uint<1> axis_dataTransferMarker = 0;
ap_uint<1> aximl_dataTransferMarker = 0;


// initialize data to write to FIFO
for(block_cnt = 0; block_cnt < NUM_TEST_BLOCKS; block_cnt++) {
  for(sample_cnt = 0; sample_cnt < BLOCK_SIZE; sample_cnt++) {
    	test_data[sample_cnt+block_cnt*BLOCK_SIZE] = (block_cnt << 16) + sample_cnt;
  } //
} // end block

// run test loop
for(block_cnt = 0; block_cnt < NUM_TEST_BLOCKS; block_cnt++) {
  for(sample_cnt = 0; sample_cnt < BLOCK_SIZE; sample_cnt++) {
	  str_in << test_data[sample_cnt+block_cnt*BLOCK_SIZE];
  } // sample_cnt
	  // call function under test

#ifdef AXI_MASTER
		stream2pingPong(str_in,
					  &axi_interrupt, 
					  axi_cpuRelease, 
					  pingPongBuffer, 
					  aximl_dataTransferMarker,
					  m_axi_dst_offset
					  );
#else
		stream2pingPong(str_out, 
					  str_in,
					  &axi_interrupt, 
					  axi_cpuRelease, 
					  axis_dataTransferMarker
					  );
#endif

	if (axi_interrupt) { // && !axi_cpuRelease) // for C sim only - do not check for !axi_cpuRelease
		cout << "----------------------  AXI STREAM INTERRUPT SET -------------------------\n";
		for(sample_cnt_test = 0; sample_cnt_test < BLOCK_SIZE; sample_cnt_test++) { // test to verify FIFO data matches test_data
#ifdef AXI_MASTER
			if ( (pingPongBuffer[sample_cnt_test] != test_data[sample_cnt_test+block_cnt*BLOCK_SIZE]) )
#else
			sample_out = str_out.read(); // pop output value off stream
			// cout << "0x" << setfill('0')  << setw(6) << hex << sample_out << " ";
			if ( (sample_out != test_data[sample_cnt_test+block_cnt*BLOCK_SIZE]) ) 
#endif
			{
				PF_test = FAIL;
				cout << "0x" << setfill('0')  << setw(6) << hex << sample_out << " ";
				cout << "-----------------------------------------------\n";
				cout << "FAILED to Match AXI Stream Memory Location \n";
				cout << "-----------------------------------------------\n";
				break;
			}
		}
		if (PF_test == PASS) {
			cout << endl;
			cout << "-----------------------------------------------\n";
			cout << "AXI Stream Memory Locations matched test_data \n";
			cout << "BLOCK COUNT 0x" << setfill('0')  << setw(6) << hex << block_cnt << " \n";
			cout << "-----------------------------------------------\n";

		}
        cout << endl;
		axi_cpuRelease = 1;
	  } // axi_interrupt
	  else axi_cpuRelease = 0;

} // block_cnt

// print sysmem
cout << "-----------------------------------------------\n";
cout << "-----------------------------------------------\n";

if (PF_test == FAIL) {
	cout << endl;
	cout << "FIFO TEST FAILED \n";
}
else
	cout << "FIFO TEST PASSED \n";
	cout << "-----------------------------------------------\n";
	cout << "-----------------------------------------------\n";
	cout << endl;

return PF_test;

} // end function main