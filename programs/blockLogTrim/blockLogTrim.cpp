#include <iostream>
#include <iomanip>
#include <fstream>
#include <unistd.h>         //truncate(char* path, u64 newLen)
using namespace std;

uint32_t reverseBytes(uint32_t x) {      //convert between big and little endian
    uint32_t rval;
    char* src= (char*)&x;
    char* dst= (char*)&rval;
    dst[0]= src[3];
    dst[1]= src[2];
    dst[2]= src[1];
    dst[3]= src[0];
    return rval;
}

char const* help=
    "This utility truncates blocks.log and blocks.index after a given block number.\n"
    "First arg is directory containing the two files then -b arg for last block to keep.\n"
    "Example: blockLogTrim eos_data/blocks -b30111222\n";

struct __attribute__((packed)) BlockHead {    //first 46 bytes of each block
  uint32_t timestamp;
  uint64_t prodname;
  uint16_t confirmed;
  uint32_t blknum;           //low 32 bits of previous blockid, is block number, is big endian
  uint32_t previous;         //high 32 bits
  uint64_t blkid[3];         //rest of 256 bit blockid for previous block
} blkHead;

char path[1024];

int main(int argc, char** argv) {
    uint32_t n;                     //last block to keep
    if (argc < 3 || argv[2][0]!='-' || argv[2][1]!='b' || !(n = atoi(argv[2] + 2))) {
        cout << help;
        return 1;
    }
    strncpy(path, argv[1], 1010);     //leave enough room for "/blocks.index"
    char *file = path + strlen(path);
    if (file > path && (file[-1]!='/'))   //add '/' if needed
        *file++ = '/';

    cout << "will truncate blocks.log after block " << n << '\n';
    //read blocks.log to see if version 1 or 2 and get firstblocknum (implicit 1 if version 1)
    filebuf fin0;
    strcpy(file, "blocks.log");
    fin0.open(path, ios::in | ios::binary);
    if (!fin0.is_open()) {
        cout << "cannot open " << path << '\n';
        return 2;
    }
    uint32_t version=0, firstBlock;
    fin0.sgetn((char*)&version,sizeof(version));
    if (version == 1)
        firstBlock= 1;
    else if (version==2)
        fin0.sgetn((char*)&firstBlock,sizeof(firstBlock));
    else {
        cout << "unknown version " << version << " so abort\n";
        return 3;
    }
    cout << "blocks.log version= " << version << " first block= " << firstBlock << '\n';
    if (n <= firstBlock) {
        cout << "already <= " << n << " blocks so nothing to do\n";
        return 4;
    }

    //open blocks.index and try to seek to 8*(n-firstBlock)
    filebuf fin1;
    strcpy(file,"blocks.index");
    fin1.open(path,ios::in|ios::binary);
    if (!fin1.is_open()) {
        cout << "cannot open " << path << '\n';
        return 5;
    }
    uint64_t indexPos= 8*(n-firstBlock);
    uint64_t endPos= fin1.pubseekoff(0,ios::end);
    if (endPos-8 < indexPos) {
        cout << "blocks.index is too short for seek to block " << n << " so abort\n";
        return 6;
    }
    //read filepos of block n and block n+1 from blocks.index
    fin1.pubseekoff(indexPos,ios::beg);
    uint64_t fpos0, fpos1;
    fin1.sgetn((char*)&fpos0,sizeof(fpos0));
    fin1.sgetn((char*)&fpos1,sizeof(fpos1));
    fin1.close();
    cout << "blocks.index says block " << n << " is at position " << fpos0 << " in blocks.log with next block at position " << fpos1 << '\n';

    //read blocks.log and verify block number at file position fpos0 == n
    fin0.pubseekoff(fpos0,ios::beg);
    fin0.sgetn((char*)&blkHead,sizeof(blkHead));
    uint32_t bnum= reverseBytes(blkHead.blknum)+1;   //convert from big endian to little endian, add 1 since prior block
    cout << "in blocks.log at position " << fpos0 << " find block number " << bnum;
    fin0.close();
    if (bnum!=n) {
        cout << "\nblocks.index does not agree with blocks.log so abort\n";
        return 7;
    } else {
        cout << " as expected.\n";
    }

    strcpy(file,"blocks.log");
    if (truncate(path,fpos1)) {
        cout << "truncate blocks.log fails so abort\n";
        return 8;
    } else {
        cout << "blocks.log has been truncated to " << fpos1 << " bytes\n";
    }
    strcpy(file,"blocks.index");
    indexPos+= sizeof(uint64_t);             //advance to after record for block n
    if (truncate(path,indexPos)) {
        cout << "truncate blocks.index fails so abort\n";
        return 9;
    } else {
        cout << "blocks.index has been truncated to " << indexPos << " bytes\n";
    }
}
