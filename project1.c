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

struct Transaction {
	int t_ID;
	int c_ID;
	int op;
	int product_ID;
	int numOfProduct;
	int day;
	struct Transaction *next;
};

struct Seller_Request {
	int c_ID;
	struct Transaction *t;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
};

int numOfSellers;
struct Product *products;
struct C_Account *c_accounts;
struct Transaction *transactions;
struct Seller_Request *seller_requests;

void *seller_thread(void *ptr) {
	struct Seller_Request *request = (struct Seller_Request*)ptr; 
	while(1) {
		while(request->t != NULL) {
			
			pthread_cond_signal(&(request->cv));
		}
	}
}

void *customer_thread(void *ptr) {
	struct C_Account *c_account = NULL;
	memcpy(c_account,ptr,sizeof(struct C_Account));
	struct Transaction *t = NULL;
//	t = prepare_transaction(c_account);
	int i;
	while(1) {
		for(i=0;i<numOfSellers;i++) {
			if(pthread_mutex_trylock(&(seller_requests[i].mutex))==0) {
				memcpy(seller_requests[i].t,t,sizeof(struct Transaction));
				pthread_cond_wait(&(seller_requests[i].cv),&(seller_requests[i].mutex));
				
			}
		}
	}
}

int main() {
	FILE *fp;
	int i,numOfCustomers,days,numOfProductTypes,num;
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
	seller_requests = malloc(numOfSellers*sizeof(struct Seller_Request));
	
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
	
	pthread_t *seller = malloc(numOfSellers*sizeof(pthread_t));
	pthread_t *customer = malloc(numOfCustomers*sizeof(pthread_t));
	
	for(i=0;i<numOfSellers;i++) {
		pthread_create(&seller[i],NULL,seller_thread,&seller_requests[i]);
	}
	
	for(i=0;i<numOfCustomers;i++) {
		pthread_create(&customer[i],NULL,customer_thread,&c_accounts[i]);
	}
	
	for(i=0;i<numOfSellers;i++) pthread_join(seller[i],NULL);
	for(i=0;i<numOfCustomers;i++) pthread_join(customer[i],NULL);
}