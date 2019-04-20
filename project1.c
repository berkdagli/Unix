#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct C_Account {
	int c_ID;
	int allowed_op;
	int allowed_res;
};

struct Product {
	int id;
	int reservedCount;
	int count;
	int *reservedList;
};

struct Product *products;
struct C_Account *c_accounts;

void *seller(void *ptr) {

}

void *customer(void *ptr) {
	
}

int main() {
	FILE *fp;
	int i,numOfSellers,numOfCustomers,days,numOfProductTypes,num;
	int num1, num2, num3;
	char line[20];
	fp = fopen("input.txt","r");
	
	fgets(line,sizeof(line),fp);
	numOfCustomers = atoi(line);
	fgets(line,sizeof(line),fp);
	numOfSellers = atoi(line);
	fgets(line,sizeof(line),fp);
	days = atoi(line);
	fgets(line,sizeof(line),fp);
	numOfProductTypes = atoi(line);
	
	c_accounts = malloc(numOfCustomers*sizeof(struct C_Account));
	products = malloc(numOfProductTypes*sizeof(struct Product));
	
	for(i=0;i<numOfProductTypes;i++) {
		fgets(line,sizeof(line),fp);
		num = atoi(line);
		products[i].id = i;
		products[i].count = num;
		products[i].reservedList = malloc(numOfCustomers*sizeof(int));
	}
	for(i=0;i<numOfCustomers;i++) {
		fgets(line,sizeof(line),fp);
		num1 = atoi(strtok(line," ")); // customer_id
		num2 = atoi(strtok(NULL," ")); // # of op allowed
		num3 = atoi(strtok(NULL," ")); // # of reservable products
		c_accounts[i].c_ID = num1;
		c_accounts[i].allowed_op = num2;
		c_accounts[i].allowed_res = num3;
	}
}