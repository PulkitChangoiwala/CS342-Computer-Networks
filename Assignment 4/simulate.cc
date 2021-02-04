#include "core.h"

int main(){
	int choice = 1;
	int res;
	do{
		std::cout<<"Choose one of the two: Part 1 or Part 2\n";
		std::cout<<"1. Part A of assignment\n";
		std::cout<<"2. Part B of assignment \n";
		std::cin>>res;
		if(res==1)
		{
			singleFlow();
		}
		else 
		{
			multiFlow();
		}
		std::cout<<"Do you want to perform another simulation? (1/0) \n";
		std::cin>>choice;
	}while(choice==1);
	std::cout<<"Thank you!"<<std::endl;
	return 0;
}
