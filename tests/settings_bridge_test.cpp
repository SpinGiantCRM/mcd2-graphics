#include "settings_bridge.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char **argv){unsigned tested=0;for(int k=1;k<argc;k++){std::ifstream f(argv[k],std::ios::binary);std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});mcd2ui::Settings s;if(!mcd2ui::decode(b,s))return 1;std::cout<<"valid revision="<<s.revision<<" mode="<<s.mode<<" scale="<<s.scale<<"\n";for(size_t n=0;n<b.size();n++){std::vector<uint8_t> cut(b.begin(),b.begin()+n);if(mcd2ui::decode(cut,s))return 2;++tested;}auto bad=b;bad[0]^=1;if(mcd2ui::decode(bad,s))return 3;bad=b;bad.push_back(0);if(mcd2ui::decode(bad,s))return 4;}
mcd2ui::Mailbox q;mcd2ui::Settings a;a.revision=3;if(!q.offer(a))return 5;a.revision=2;if(q.offer(a))return 6;a.revision=4;a.mode=2;if(!q.offer(a)||!q.take(a)||a.revision!=4||a.mode!=2||q.take(a))return 7;std::cout<<"rejected truncations="<<tested<<"; latest-valid mailbox PASS\n";}
