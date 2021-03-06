/*
 * transactions.h
 *
 *  Created on: Dec 17, 2015
 *      Author: xiaoxin
 */

#ifndef TRANSACTIONS_H_
#define TRANSACTIONS_H_

#include <stdint.h>

//200,000(6) unique ID
#define ITEM_ID (uint64_t)1000000

//2*w unique ID
#define WHSE_ID (uint64_t)10000

//20 unique ID
#define DIST_ID (uint64_t)100

//96,000(5) unique ID
#define CUST_ID (uint64_t)100000

//10,000,000(8) unique ID
#define ORDER_ID (uint64_t) 100000//100000000

//5-15 items per order.
#define ORDER_LINES (uint64_t)100

//quantity of each item is between (1, 10)
#define ITEM_QUANTITY 100

//'0' for bad credit, '1' for good credit.
#define CUST_CREDIT 10

//discount from 0 to 5.
#define CUST_DISCOUNT 10

/*
 * table_id: warehouse:0, item:1, stock:2, district:3, customer:4, history:5,
 * order:6, new_order:7, order_line:8.
 */
#define Warehouse_ID 0
#define Item_ID 1
#define Stock_ID 2
#define District_ID 3
#define Customer_ID 4
#define History_ID 5
#define Order_ID 6
#define NewOrder_ID 7
#define OrderLine_ID 8

typedef enum
{
	NEW_ORDER,
	PAYMENT,
	DELIVERY,
	ORDER_STATUS,
	STOCK_LEVEL
}TransactionsType;

typedef struct TransState
{
	int trans_commit;
	int trans_abort;
	int NewOrder;
	int Payment;
	int Delivery;
	int Stock_level;
	int Order_status;

	int runabort;
	int endabort;
	int otherabort;

	int NewOrder_C;
	int Payment_C;
	int Stock_level_C;
}TransState;

extern int newOrderTransaction(int w_id, int d_id, int c_id, int o_ol_cnt, int o_all_local, int *itemIDs, int *supplierWarehouseIDs, int *orderQuantities, int* node_id, int node_num);

extern int paymentTransaction(int w_id, int c_w_id, int h_amount, int d_id, int c_d_id, int c_id, int* node_id, int node_num);

extern int deliveryTransaction(int w_id, int o_carrier_id, int* node_id, int node_num);

extern int orderStatusTransaction(int w_id, int d_id, int c_id, int* node_id, int node_num);

extern int stockLevelTransaction(int w_id, int d_id, int threshold, int* node_id, int node_num);

extern void executeTransactions(int numTransactions, int terminalWarehouseID, int terminalDistrictID, TransState* StateInfo);

extern uint64_t LoadData(void);
#endif /* TRANSACTIONS_H_ */
