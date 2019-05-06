#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct C_Account {
	int c_ID;
	int allowed_op;
	int allowed_res;
};

struct Product {
	int id;
	int initial_count;
	int current_count;
	pthread_mutex_t mutex;
};

struct Transaction {
	int t_ID;
	int c_ID;
	int op;
	int product_ID;
	int numOfProduct;
	int day;
	struct Transaction *cancelled_transaction;
	struct Transaction *next;
};

struct Seller_Request {
	int c_ID;
	struct Transaction *t;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	pthread_mutex_t cond_mutex;
	int customer_wait;
};

int numOfSellers,numOfProductTypes,days;
int t_count=0;
int currentday=1;
pthread_mutex_t transaction_mutex; //mutex for appending transaction to the transaction list
struct Product *products;
struct C_Account *c_accounts;
struct Transaction *transactions;
struct Seller_Request *seller_requests;
struct Transaction *prepare_transaction(struct C_Account *c);
struct Transaction *transaction_to_cancel(int c_ID);
struct C_Account *find_c_account_by_ID(int id);
void append_transaction(struct Transaction *t);

void *seller_thread(void *ptr) {
	struct Seller_Request *request = (struct Seller_Request*)ptr;
	struct Transaction *t = NULL;
	struct C_Account *c_acc = NULL;
	struct Product *p = NULL;
	while(1) {
		if(request->t != NULL) {
			t = request->t;
			c_acc = find_c_account_by_ID(request->c_ID);
			p = &products[t->product_ID];
			c_acc->allowed_op--;
			if(t->op == 0) { //handle buy operation
				pthread_mutex_lock(&(p->mutex));
				if(p->current_count >= t->numOfProduct) {
					p->current_count -= t->numOfProduct;
				}
				else {
					free(t);
					request->t = NULL;
				}
				pthread_mutex_unlock(&(p->mutex));
			}
			else if(t->op == 1) { //handle reserve operation
				pthread_mutex_lock(&(p->mutex));
				if(p->current_count >= t->numOfProduct) {
					p->current_count -= t->numOfProduct;
					c_acc->allowed_res -= t->numOfProduct;
				}
				else {
					free(t);
					request->t = NULL;
				}
				pthread_mutex_unlock(&(p->mutex));
			}
			else { //handle cancel reservation operation
				pthread_mutex_lock(&(p->mutex));
				p->current_count += t->cancelled_transaction->numOfProduct;
				pthread_mutex_unlock(&(p->mutex));
			}
			if(request->t != NULL) { //If transaction accepted, append to the transactions list
				pthread_mutex_lock(&transaction_mutex);
				append_transaction(t);
				pthread_mutex_unlock(&transaction_mutex);
				free(t);
				request->t = NULL;
			}
			pthread_mutex_lock(&(request->cond_mutex));
			pthread_cond_signal(&(request->cv));
			request->customer_wait = 0;
			pthread_mutex_unlock(&(request->cond_mutex));
		}
	}
}

void *customer_thread(void *ptr) {
	struct C_Account *c_account = malloc(sizeof(struct C_Account));
	memcpy(c_account,ptr,sizeof(struct C_Account)); //c_account is the initial customer account
	struct Transaction *t;
	int day=1;
	int i;
	int count=1;
	while(((struct C_Account*)ptr)->allowed_op > 0) {
		t = prepare_transaction(ptr);
		while(t != NULL) {
			for(i=0;i<numOfSellers;i++) {
				if(pthread_mutex_trylock(&(seller_requests[i].mutex))==0) {
					seller_requests[i].c_ID = c_account->c_ID;
					seller_requests[i].customer_wait = 1;
					seller_requests[i].t = malloc(sizeof(struct Transaction));
					memcpy(seller_requests[i].t,t,sizeof(struct Transaction));
					pthread_mutex_lock(&(seller_requests[i].cond_mutex));
					while(seller_requests[i].customer_wait) {
						pthread_cond_wait(&(seller_requests[i].cv),&(seller_requests[i].cond_mutex));
					}
					pthread_mutex_unlock(&(seller_requests[i].cond_mutex));
					pthread_mutex_unlock(&(seller_requests[i].mutex));
					free(t);
					t = NULL;
					break;
				}
			}
		}
	}
//	fprintf(stderr,"----------customer-%d bitirdi----------\n",c_account->c_ID);
}

void append_transaction(struct Transaction *t) {
	struct Transaction *temp = transactions;
	if(temp==NULL) {
		transactions = malloc(sizeof(struct Transaction));
		memcpy(transactions,t,sizeof(struct Transaction));
	}
	else {
		while(temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = malloc(sizeof(struct Transaction));
		memcpy(temp->next,t,sizeof(struct Transaction));
	}
}

struct Transaction *prepare_transaction(struct C_Account *c) {
	int flag,op;
	struct Transaction *t = NULL;
	t = malloc(sizeof(struct Transaction));
	pthread_mutex_lock(&transaction_mutex);
	t->t_ID  = t_count;
	t_count++;
	pthread_mutex_unlock(&transaction_mutex);
	t->c_ID = c->c_ID;
	t->next = NULL;
	
	do {
		flag = 0;
		op = rand()%3; //0:buy, 1:reservation, 2:cancel reservation
		if(op == 0) {
			t->op = 0;
			t->numOfProduct = (rand()%10)+1;
			t->product_ID = rand()%numOfProductTypes;
		}
		else if(op == 1) {
			if(c->allowed_res > 0) {
				t->op = 1;
				t->numOfProduct = (rand()%c->allowed_res)+1;
				t->product_ID = rand()%numOfProductTypes;
			}	
			else flag = 1;
		}
		else {
			if((t->cancelled_transaction = transaction_to_cancel(c->c_ID)) != NULL) {
				t->op = 2;
			}
			else { 
				flag = 1;
			}
		}
	}
	while(flag);
	
	t->day = currentday;
	
	return t;
}

struct C_Account *find_c_account_by_ID(int id) {
	int i=0;
	while(c_accounts[i].c_ID != id) {
		i++;
	}
	return &c_accounts[i];
}

int sizeOfList(struct Transaction *t) {
	struct Transaction *temp = t;
	int size=0;
	if(t != NULL) {
		while(temp) {
			size++;
			temp = temp->next;
		}
		return size;
	}
	return 0;
}

struct Transaction *transaction_to_cancel(int c_ID) {
	int count=0;
	struct Transaction *t[20];
	struct Transaction *temp = transactions;
	while((temp != NULL) && (temp->day <= currentday)) {
		if((temp->day == currentday) && (temp->c_ID == c_ID) && (temp->op == 1)) {
			t[count] = temp;
			count++;
		}
		temp = temp->next;
	}
	if(count>0) return t[rand()%count];
	else return NULL;
	
}



struct Transaction *find_transaction_by_ID(int id) {
	struct Transaction *t = transactions;
	while(t->t_ID != id) {
		t = t->next;
	}
	return t;
}

void print_log() {
	struct Transaction *t = transactions;
	int day = 1;
	while(t != NULL) {
		printf("%d\t%d\t%d\n",t->c_ID,t->op,t->day);
		t = t->next;
	}
}

int main() {
	FILE *fp;
	int i,numOfCustomers,num;
	int num1, num2, num3;
	char line[20];
	srand(time(NULL));
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
		products[i].initial_count = num;
		products[i].current_count = num;
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
	
//	for(i=0;i<numOfSellers;i++) pthread_join(seller[i],NULL);
	for(i=0;i<numOfCustomers;i++) pthread_join(customer[i],NULL);
	print_log();
}