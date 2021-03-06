/*
 * data_record.c
 *
 *  Created on: Dec 7, 2015
 *      Author: xiaoxin
 */
#include<assert.h>
#include"data_record.h"
#include"mem.h"
#include"thread_global.h"
#include"data_am.h"
#include"trans.h"
#include "communicate.h"


#define DataMemMaxSize 128*1024

void CommitInsertData(int table_id, int index, TimeStampTz ctime, int nid)
{
	THREAD* threadinfo;
	int index2;
	threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
	index2=threadinfo->index;

	int lindex;
	lindex = GetLocalIndex(index2);

	if((Send4(lindex, nid, cmd_commitinsert, table_id, index, ctime)) == -1)
		printf("commit insert send error!\n");
	if((Recv(lindex, nid, 1)) == -1)
		printf("commit insert recv error!\n");
}

void CommitUpdateData(int table_id, int index, TimeStampTz ctime, int nid)
{
	int index2;
	THREAD* threadinfo;
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
	index2=threadinfo->index;

	int lindex;
	lindex = GetLocalIndex(index2);

	if (Send4(lindex, nid, cmd_commitupdate, table_id, index, ctime) == -1)
		printf("commit update send error\n");
    if((Recv(lindex, nid, 1)) == -1)
    	printf("commit update recv error\n");
}

void CommitDeleteData(int table_id, int index, TimeStampTz ctime, int nid)
{
	int index2;
	THREAD* threadinfo;
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
	index2=threadinfo->index;

	int lindex;
	lindex = GetLocalIndex(index2);

	if (Send4(lindex, nid, cmd_commitupdate, table_id, index, ctime) == -1)
		printf("commit delete send error\n");
    if((Recv(lindex, nid, 1)) == -1)
    	printf("commit delete recv error\n");
}

void AbortInsertData(int table_id, int index, int nid)
{
	int index2;
	THREAD* threadinfo;
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
	index2=threadinfo->index;

	int lindex;
	lindex = GetLocalIndex(index2);

	if (Send3(lindex, nid, cmd_abortinsert, table_id, index) == -1)
		printf("abort insert send error\n");
    if((Recv(lindex, nid, 1)) == -1)
    	printf("abort insert recv error\n");
}

void AbortUpdateData(int table_id, int index, int nid)
{
	bool isdelete = false;
	int index2;
	THREAD* threadinfo;
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
	index2=threadinfo->index;

	int lindex;
	lindex = GetLocalIndex(index2);

	if (Send4(lindex, nid, cmd_abortupdate, table_id, index, isdelete) == -1)
		printf("abort update send error\n");
    if((Recv(lindex, nid, 1)) == -1)
    	printf("abort update recv error\n");
}

void AbortDeleteData(int table_id, int index, int nid)
{
	bool isdelete = true;
	int index2;
	THREAD* threadinfo;
    threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
	index2=threadinfo->index;

	int lindex;
	lindex = GetLocalIndex(index2);

	if (Send4(lindex, nid, cmd_abortupdate, table_id, index, isdelete) == -1)
		printf("abort delete send error\n");
    if((Recv(lindex, nid, 1)) == -1)
    	printf("abort delete recv error\n");
}

/*
 * insert a data-update-record.
 */
void DataRecordInsert(DataRecord* datard)
{
	int num;
	char* start;
	char* DataMemStart;
	DataRecord* ptr;

	/* get the thread's pointer to data memory. */
	DataMemStart=(char*)pthread_getspecific(DataMemKey);

	start=(char*)((char*)DataMemStart+DataNumSize);
	num=*(int*)DataMemStart;

	if(DataNumSize+(num+1)*sizeof(DataRecord) > DataMemSize())
	{
		printf("data memory out of space. PID: %lu\n",pthread_self());
		return;
	}

	*(int*)DataMemStart=num+1;

	/* start address for record to insert. */
	start=start+num*sizeof(DataRecord);

	/* insert the data record here. */
	ptr=(DataRecord*)start;
	ptr->type=datard->type;

	ptr->table_id=datard->table_id;
	ptr->tuple_id=datard->tuple_id;

	ptr->value=datard->value;

	ptr->index=datard->index;

	ptr->node_id = datard->node_id;
}

Size DataMemSize(void)
{
	return DataMemMaxSize;
}

void InitDataMemAlloc(void)
{
	char* DataMemStart=NULL;
	char* memstart;
	THREAD* threadinfo;
	Size size;

	/* get start address of current thread's memory. */
	threadinfo=(THREAD*)pthread_getspecific(ThreadInfoKey);
	memstart=threadinfo->memstart;

	size=DataMemSize();
	DataMemStart=(char*)MemAlloc((void*)memstart,size);

	if(DataMemStart==NULL)
	{
		printf("thread memory allocation error for data memory.PID:%lu\n",pthread_self());
		return;
	}

	/* set global variable. */
	pthread_setspecific(DataMemKey,DataMemStart);
}

void InitDataMem(void)
{
	char* DataMemStart=NULL;

	DataMemStart=(char*)pthread_getspecific(DataMemKey);

	memset(DataMemStart,0,DataMemSize());
}

/*
 * write the commit time of current transaction to every updated tuple.
 */
void CommitDataRecord(TransactionId tid, TimeStampTz ctime)
{
	int num;
	int i;
	char* DataMemStart=NULL;
	char* start;
	DataRecord* ptr;
	int table_id;
	int nid;
	uint64_t index;

	DataMemStart=(char*)pthread_getspecific(DataMemKey);
	start=DataMemStart+DataNumSize;
	num=*(int*)DataMemStart;

	/* deal with data-update record one by one. */
	for(i=0;i<num;i++)
	{
		ptr=(DataRecord*)(start+i*sizeof(DataRecord));
		table_id=ptr->table_id;
		index=ptr->index;
		nid = ptr->node_id;
		switch(ptr->type)
		{
		case DataInsert:
			CommitInsertData(table_id, index, ctime, nid);
			break;
		case DataUpdate:
			CommitUpdateData(table_id, index, ctime, nid);
			break;
		case DataDelete:
			CommitDeleteData(table_id, index, ctime, nid);
			break;
		default:
			printf("shouldn't arrive here.\n");
		}
	}
}

/*
 * rollback all updated tuples by current transaction.
 */
void AbortDataRecord(TransactionId tid, int trulynum)
{
	int num;
	int i;
	char* DataMemStart=NULL;
	char* start;
	DataRecord* ptr;
	int table_id;
	int nid;
	uint64_t index;

	DataMemStart=(char*)pthread_getspecific(DataMemKey);
	start=DataMemStart+DataNumSize;
	/* transaction abort in function CommitTransaction. */
	if(trulynum==-1)
		num=*(int*)DataMemStart;
	/* transaction abort in function AbortTransaction. */
	else
		num=trulynum;

	for(i=0;i<num;i++)
	{
		ptr=(DataRecord*)(start+i*sizeof(DataRecord));
		table_id=ptr->table_id;
		index=ptr->index;
		nid = ptr->node_id;
		switch(ptr->type)
		{
		case DataInsert:
			AbortInsertData(table_id, index, nid);
			break;
		case DataUpdate:
			AbortUpdateData(table_id, index, nid);
			break;
		case DataDelete:
			AbortDeleteData(table_id, index, nid);
			break;
		default:
			printf("shouldn't get here.\n");
		}
	}
}

/*
 * sort the transaction's data-record to avoid dead lock between different
 * update transactions.
 * @input:'dr':the start address of data-record, 'num': number of data-record.
 */
void DataRecordSort(DataRecord* dr, int num)
{
	/* sort according to the table_id and tuple_id and node_id */
	DataRecord* ptr1, *ptr2;
	DataRecord* startptr=dr;
	DataRecord temp;
	int i,j;

	for(i=0;i<num-1;i++)
		for(j=0;j<num-i-1;j++)
		{
			ptr1=startptr+j;
			ptr2=startptr+j+1;
			if((ptr1->node_id > ptr2->node_id) || ((ptr1->node_id == ptr2->node_id) && (ptr1->table_id > ptr2->table_id)) || ((ptr1->node_id == ptr2->node_id) && (ptr1->table_id == ptr2->table_id) && (ptr1->tuple_id > ptr2->tuple_id)))
			{
				temp.table_id=ptr1->table_id;
				temp.tuple_id=ptr1->tuple_id;
				temp.type=ptr1->type;
				temp.value=ptr1->value;
				temp.index=ptr1->index;
				temp.node_id = ptr1->node_id;

				ptr1->table_id=ptr2->table_id;
				ptr1->tuple_id=ptr2->tuple_id;
				ptr1->type=ptr2->type;
				ptr1->value=ptr2->value;
				ptr1->index=ptr2->index;
				ptr1->node_id=ptr2->node_id;

				ptr2->table_id=temp.table_id;
				ptr2->tuple_id=temp.tuple_id;
				ptr2->type=temp.type;
				ptr2->value=temp.value;
				ptr2->index=temp.index;
				ptr2->node_id=temp.node_id;
			}
		}
}

/*
 * @return:'1' for visible inner own transaction, '-1' for invisible inner own transaction,
 * '0' for first access the tuple inner own transaction.
 */
TupleId IsDataRecordVisible(char* DataMemStart, int table_id, TupleId tuple_id, int node_id)
{
	int num,i;
	char* start=DataMemStart+DataNumSize;
	num=*(int*)DataMemStart;
	DataRecord* ptr;

	for(i=num-1;i>=0;i--)
	{
		ptr=(DataRecord*)(start+i*sizeof(DataRecord));
		if(ptr->tuple_id == tuple_id && ptr->table_id == table_id && ptr->node_id == node_id)
		{
			if(ptr->type == DataDelete)
			{
				/* the tuple has been deleted by current transaction. */
				return -1;
			}
			else
			{
				return ptr->value;
			}
		}
	}

	/* first access the tuple. */
	return 0;
}



