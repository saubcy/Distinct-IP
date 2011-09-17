#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <algorithm>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

struct IP_UNIT {
	long long len;
	unsigned long start_ip;
	unsigned long end_ip;
	std::string  province;
	std::string  city;
	std::string  isp;
};

void vSplitString( string strSrc ,vector<string>& vecDest, char cSeparator);

void loadIpData(vector<IP_UNIT>& ip_saw, char* filename);

int binarySearch(vector<unsigned long long>& a,unsigned long long x,unsigned int n);

bool compareLen(const IP_UNIT& a, const IP_UNIT& b) {
	return a.len < b.len;
}

bool compareStart(const IP_UNIT& a, const IP_UNIT& b) {
	if ( a.start_ip == b.start_ip ) {
		return a.end_ip < b.end_ip;
	}
	return a.start_ip < b.start_ip;
}

void insert(IP_UNIT& node, vector<IP_UNIT>& total, vector<unsigned long long>& vertex);

void dumpVertex(vector<unsigned long long>& vertex) {
	for (unsigned int i=0; i<vertex.size(); ++i ) {
		printf("%llu,",vertex[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[]) {
	
	//ofstream outfile(argv[2]);
	
	if ( argc < 2 ) {
		printf("useage: ip_distinct [inputfile] [outputfile]\n");
		return -1;
	}
	
	vector<IP_UNIT> ip_saw;
	printf("loading data...\n");
	loadIpData(ip_saw, argv[1]);
	printf("data load finished size: %d\n", ip_saw.size());
	
	// 按照IP段由短到长进行排序
	printf("sorting...\n");
	sort(ip_saw.begin(), ip_saw.end(), compareLen);
	printf("sort finished...\n");
	
	// 将节点逐个插入
	vector<IP_UNIT> total;		// 节点集合
	vector<unsigned long long> vertex;				// 顶点集合
	int sawSize = ip_saw.size();
	for (int i=0; i<ip_saw.size(); ++i) {
		printf("processing: %d/%d\n", i+1, sawSize);
		insert(ip_saw[i], total, vertex);
	}
	
	// 重新排序
	char tmp[] = "/temp/ip.log"; 
	char* outputfile = NULL;
	if ( 3 == argc ) {
		outputfile = argv[2];
	} else {
		outputfile = tmp;
	}
	ofstream outfile(outputfile);
	printf("sorting...\n");
	sort(total.begin(), total.end(), compareStart);
	printf("sort finished...\n");
	for (unsigned int i=0; i<total.size(); ++i) {
		
		if ( 3 == argc ) {
			outfile <<total[i].start_ip/10
				<<"|"<<total[i].end_ip/10
				<<"|"<<total[i].province
				<<"|"<<total[i].city
				<<"|"<<total[i].isp
				<<endl;
		} else {
			printf("%llu|%llu|%s|%s|%s\n", 
			total[i].start_ip/10, total[i].end_ip/10,
		  total[i].province.c_str(), total[i].city.c_str(),
		  total[i].isp.c_str());
		}
	}
	printf("all finished...\n");
	if ( 3 == argc ) {
		outfile.close();
	}
	
	return 0;
}

void loadIpData(vector<IP_UNIT>& ip_saw, char* filename) {
	
	ifstream infile(filename, ios::in);	
	map<unsigned long long, int> tags;		// 存储已经存在的开始结尾标记，用来保证所有标记不重复
	
	vector<IP_UNIT> ip_from_file;
	
	string str = "";
	unsigned int count = 0;
	while(getline(infile,str,'\n')) {	// 首先直接加载文件中的数据
		// printf("%u\n", ++count);
		if ( str.length() < 1 ) {
			break;
		}
		vector<string> vecDest;
		vSplitString(str, vecDest, '|');
		
		IP_UNIT unit;
		unit.start_ip = atoll(vecDest[0].c_str())*10;	// 为了处理开始IP与结束IP相同的情况，将原IP放大十倍
		unit.end_ip = atoll(vecDest[1].c_str())*10+1;	// 结束IP统一加一
		unit.len = unit.end_ip  - unit.start_ip;
		if ( unit.len < 0 ) {
			continue;
		}
		
		unit.province = vecDest[2];
		unit.city = vecDest[3];
		unit.isp = vecDest[4];
		
		ip_from_file.push_back(unit);
	}
	infile.close();
	
	// 对原始IP记录对长度进行排序
	sort(ip_from_file.begin(), ip_from_file.end(), compareLen);
	
	// 遍历IP记录，按照区段从窄到宽的顺序压缩有重叠的IP段
	for(unsigned int i=0; i<ip_from_file.size(); ++i) {
		
		//printf("trace loadIpData deal: (%llu,%llu,%lld)\n", 
		//	ip_from_file[i].start_ip, ip_from_file[i].end_ip, ip_from_file[i].len);
		
		IP_UNIT unit;
		unit.start_ip = ip_from_file[i].start_ip;	// 为了处理开始IP与结束IP相同的情况，将原IP放大十倍
		unit.end_ip = ip_from_file[i].end_ip;	// 结束IP统一加一
		
		bool isContinue = false;
		
		// 判断是否有标记重复，若重复将开始标记前移，结束标记后移
		while( tags.find(unit.start_ip) != tags.end() ) {
			unit.start_ip += 10;		// 按照放大前的数字移动
			if ( unit.start_ip > unit.end_ip ) {
				isContinue = true;
				break;
			}
		}
		if ( isContinue ) {	// 该数据段已经没有存在的必要
			continue;
		}
		
		while( tags.find(unit.end_ip) != tags.end() ) {
			unit.end_ip -= 10;
			if ( unit.end_ip < unit.start_ip ) {
				isContinue = true;
				break;
			}
		}
		unit.len = unit.end_ip - unit.start_ip;
		if ( isContinue ) {	// 该数据段已经没有存在的必要
			continue;
		}
		
		unit.province = ip_from_file[i].province;
		unit.city = ip_from_file[i].city;
		unit.isp = ip_from_file[i].isp;
		
		ip_saw.push_back(unit);
		tags.insert(make_pair(unit.start_ip, 0));
		tags.insert(make_pair(unit.end_ip, 0));
		//printf("trace loadIpData insert: (%llu,%llu,%lld)\n", 
		//	unit.start_ip, unit.end_ip, unit.len);
	}
	
#if 0 // 未排序的原始处理方式
	string str = "";
	unsigned int count = 0;
	while(getline(infile,str,'\n')) {
		// printf("%u\n", ++count);
		if ( str.length() < 1 ) {
			printf("break!!!!!\n");
			break;
		}
		vector<string> vecDest;
		vSplitString(str, vecDest, '|');
		printf("read: %s\n", str.c_str());
		
		IP_UNIT unit;
		unit.start_ip = atoll(vecDest[0].c_str())*10;	// 为了处理开始IP与结束IP相同的情况，将原IP放大十倍
		unit.end_ip = atoll(vecDest[1].c_str())*10+1;	// 结束IP统一加一
		
		bool isContinue = false;
		
		// 判断是否有标记重复，若重复将开始标记前移，结束标记后移
		while( tags.find(unit.start_ip) != tags.end() ) {
			unit.start_ip += 10;		// 按照放大前的数字移动
			if ( unit.start_ip > unit.end_ip ) {
				isContinue = true;
				break;
			}
		}
		if ( isContinue ) {	// 该数据段已经没有存在的必要
			continue;
		}
		
		while( tags.find(unit.end_ip) != tags.end() ) {
			unit.end_ip -= 10;
			if ( unit.end_ip < unit.start_ip ) {
				isContinue = true;
				break;
			}
		}
		unit.len = unit.end_ip - unit.start_ip;
		if ( isContinue ) {	// 该数据段已经没有存在的必要
			continue;
		}
		
		unit.province = vecDest[2];
		unit.city = vecDest[3];
		unit.isp = vecDest[4];
		
		ip_saw.push_back(unit);
		tags.insert(make_pair(unit.start_ip, 0));
		tags.insert(make_pair(unit.end_ip, 0));
		//printf("trace loadIpData insert: (%llu,%llu,%lld)\n", 
		//	unit.start_ip, unit.end_ip, unit.len);
	}
	infile.close();
#endif
}

void vSplitString(string strSrc ,vector<string>& vecDest ,char cSeparator ) {
    if( strSrc.empty() )
        return ;
 
      string::size_type size_pos = 0;
      string::size_type size_prev_pos = 0;
       
      while( (size_pos = strSrc.find_first_of( cSeparator ,size_pos)) != string::npos)
      {
           string strTemp=  strSrc.substr(size_prev_pos , size_pos-size_prev_pos );
           vecDest.push_back(strTemp);
           size_prev_pos =++ size_pos;
      }    
     
      vecDest.push_back(&strSrc[size_prev_pos]);
}

void insert(IP_UNIT& node, vector<IP_UNIT>& total, vector<unsigned long long>& vertex) {
	
	// 首先将新顶点插入顶点集合，并重新排序
	vector<unsigned long long> tVertex = vertex;
	tVertex.push_back(node.start_ip);
	tVertex.push_back(node.end_ip);
	sort(tVertex.begin(), tVertex.end());
	
	if ( 0 == total.size() ) {	// 第一条记录直接添加
		//printf("trace: insert 0\n");
		total.push_back(node);
		vertex.push_back(node.start_ip);
		vertex.push_back(node.end_ip);
		return;
	}
	
	// 查找插入重排序后的起点位置
	unsigned int index = 
		binarySearch(tVertex, node.start_ip, tVertex.size());
	unsigned int end_ip = 0;
	unsigned int nOffset = 1;
	if ( 0 == index%2 ) {	// 起点位于偶数位置
		//printf("trace: insert 1\n");
		IP_UNIT unit;
		unit.start_ip = node.start_ip;	// 新节点的开始位置就是原节点位置
		unit.end_ip = tVertex[index+nOffset]; 
		//printf("trace: insert value_1: "
		//	"%llu\n", unit.end_ip);
		if ( node.end_ip != tVertex[index+nOffset] ) {
			unit.end_ip -= 10;
		}
		
		nOffset += 1;
		unit.province = node.province;
		unit.city = node.city;
		unit.isp = node.isp;
		if ( unit.end_ip <  unit.start_ip 
				&& (unit.end_ip+1) == unit.start_ip ) {
			unit.end_ip = unit.start_ip + 1;
		}
		unit.len = unit.end_ip - unit.start_ip;
		//printf("trace: insert value_0: "
		//	"%llu - %llu = %lld\n", 
		//	unit.end_ip, unit.start_ip, unit.len);
		if ( unit.len >= 0 ) {	// 加入新的区段信息
			// printf("trace: insert 2\n");
			total.push_back(unit);
			vertex.push_back(unit.start_ip);
			vertex.push_back(unit.end_ip);
		}
	}
	
	while ( (index+nOffset+1) < tVertex.size()
					&& tVertex[index+nOffset] 
					< node.end_ip ) {	// 继续添加
		//printf("trace: insert 3\n");
		IP_UNIT unit;
		unit.start_ip = tVertex[index+nOffset] + 10;
		nOffset += 1;	
		unit.end_ip = tVertex[index+nOffset];
		if ( node.end_ip != tVertex[index+nOffset] ) {
			unit.end_ip -= 10;
		}
		
		nOffset += 1;	
		unit.province = node.province;
		unit.city = node.city;
		unit.isp = node.isp;
		if ( unit.end_ip < unit.start_ip
			&& (unit.end_ip+1) == unit.start_ip ) {
			unit.end_ip = unit.start_ip + 1;
		}
		unit.len = unit.end_ip - unit.start_ip;
		//printf("trace: insert value_3: "
		//	"%llu - %llu = %lld\n", 
		//	unit.end_ip, unit.start_ip, unit.len);
		if ( unit.len >= 0 ) {	// 加入新的区段信息
			//printf("trace: insert 4\n");
			total.push_back(unit);
			vertex.push_back(unit.start_ip);
			vertex.push_back(unit.end_ip);
			if ( 974422016 == (unit.start_ip/10) ) {
				dumpVertex(tVertex);
				printf("insert node(%llu, %llu)\n", 
					node.start_ip, node.end_ip);
				printf("add node(%llu, %llu)\n", 
					unit.start_ip, unit.end_ip);
			}
		}	
	}
}

int binarySearch(vector<unsigned long long>& a,unsigned long long x,unsigned int n){
	int low=0,high=n-1;
 	while(low<=high){
  	int mid=(low+high)/2;
  	if(x==a[mid])
  		return mid;
  	if(x>a[mid])
   		low=mid+1;
  	else
   	high=mid-1;
 }
 return -1;
}



