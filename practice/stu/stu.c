#include "stu.h"
int find_top(student arr[] ,int n){
	int max=-1;
	for(int i=0;i<n;i++){
		if(max==-1){
			max=i;
		}else if(arr[i].score>arr[max].score){
			max=i;
		}
	}
	return max;
}
