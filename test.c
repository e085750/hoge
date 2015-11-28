#property strict

double price_width = 0.2;
double lot = 0.01;
bool first_order = TRUE;
int last_buy_ticket = -1;
int last_sell_ticket = -1;
double lower_buy_price = -1 ;
double upper_sell_price  = -1 ;
//tuka tani de henko suru
double fraction_range = 0;
//USD
int base_money = 20000;
double profit_unit = 1000;
double profit_lower = 0;

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit() {
	if (AccountCurrency() == "JPY") {
		//JPY
		base_money = 2000000;
		profit_unit = 100000;
		Print ("JPY mode start");
	} else {
		Print ("USD mode start");
	}

	return(INIT_SUCCEEDED);
  }
//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
  }
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick() {
	calcLot();

	if (first_order ) {
	  last_buy_ticket = OrderSend(Symbol(), OP_BUY, lot, Ask, 3, 0, 0, "Buy", 10, 0, Blue);
		if (OrderSelect(last_buy_ticket, SELECT_BY_TICKET, MODE_TRADES)){
			lower_buy_price  = OrderOpenPrice();
			upper_sell_price   = lower_buy_price ;
		} else {
			Print("first order error. CODE:", GetLastError());
			return;
		}

		first_order = FALSE;
		return;
	}

	payingClose();

	RefreshRates();
	if (isPositionExist(OP_BUY)) {

		if ((upper_sell_price   + price_width) <= Bid) {
			simpleOrder(OP_SELL);
		}

		if (isPositionExist(OP_BUY) && Bid <= (upper_sell_price - price_width) ) {
			simpleOrder(OP_SELL);
		}


		if (Ask <= (lower_buy_price  - price_width)) {
			simpleOrder(OP_BUY);
		}

	} else {
			simpleOrder(OP_BUY);
	}

		return;	
}

void simpleOrder(int order_type) {
	double lot_correction = 0;
	if (0 < AccountFreeMarginCheck(Symbol(), order_type, lot) ) {
		if (order_type == OP_BUY) {
			int price_level = Ask * 10;
			if ((price_level % 10 ) < fraction_range) {
				lot_correction = 0.01;
			}
			last_buy_ticket = OrderSend(Symbol(), OP_BUY, lot + lot_correction, Ask, 3, 0, 0, "Buy", 10, 0, Blue);
			if (OrderSelect(last_buy_ticket, SELECT_BY_TICKET, MODE_TRADES)) {
				lower_buy_price  = OrderOpenPrice();
				if (upper_sell_price < 0) {
					upper_sell_price = lower_buy_price;
				}
				orderInfo(last_buy_ticket);
			} else {
				printInfo();
			}
		} else if (order_type == OP_SELL) {
			int price_level = Bid * 10;
			if ((price_level % 10 ) < fraction_range) {
				lot_correction = 0.01;
			} else {
				lot_correction = 0;
			}

			if(0 < AccountFreeMarginCheck(Symbol(), OP_SELL, lot) ) {
				last_sell_ticket = OrderSend(Symbol(), OP_SELL, lot + lot_correction, Bid, 3, 0, 0, "Sell", 10, 0, Red);
				if (OrderSelect(last_sell_ticket, SELECT_BY_TICKET, MODE_TRADES)) {
					upper_sell_price   = OrderOpenPrice();
					orderInfo(last_sell_ticket);
				} else {
					printInfo();
				}
			}
		} else {
			Print("no matche order [simpelOrder() ERROR]");
		}
	}
}

bool isPositionExist(int order_type) {
	for (int i = 0; i < OrdersTotal(); i++) {
		if (OrderSelect(i, SELECT_BY_POS, MODE_TRADES)) {
			if (OrderType() == order_type) {
				return true;
			} 
		} 
	}

	return false;
}

void calcLot() {
	profit_lower = profit_unit * lot * price_width;
	double f_margin = AccountBalance();
	if (f_margin < base_money) {
		lot = 0.01;
		double tmp = base_money / f_margin;
		tmp = MathRound(tmp);
		price_width = tmp/10;
	} else {
		int int_f_margin = f_margin;
		price_width = 0.1;
		double tmp = f_margin / base_money;
		int fraction = int_f_margin % base_money;
		fraction_range = fraction / (base_money / 10);
		tmp = MathRound(tmp);
		lot = tmp / 10;
	}
	
}

void payingClose() {
	for (int i = 0; i < OrdersTotal(); i++) {
		if (OrderSelect(i, SELECT_BY_POS, MODE_TRADES)) {
			if (profit_lower < OrderProfit() ) {
				int orderType = OrderType();
				while (true) {
					double price;
					if (orderType == OP_BUY) {
						price = Bid;
					} else {// OP_SELL
						price = Ask;
					}
					if (OrderClose(OrderTicket(), OrderLots(), price, 1, Gold) != true ) {
						// failed postion close
						int lastError = GetLastError();
						Print("OrderClose() failed : ", GetLastError());
						OrderPrint();

						// if price slip, retry
						if (lastError == ERR_PRICE_CHANGED) {
							RefreshRates();
							continue;
						}
					}
					break;
				}
			}
		} else {
			Print("close order error. CODE:", GetLastError());
		}
	}

	return;
}

void printInfo() {
			Print("buy order error. CODE:", GetLastError(),
					". reverage:", AccountLeverage(),
					". account balance:", AccountBalance(),
					". orders total:", OrdersTotal(),
					". Account Free Margin:",AccountFreeMargin(),
					". Account Free Margin Mode:",AccountFreeMarginMode()
					);
	}

void orderInfo(int ticket) {
	if (OrderSelect(ticket, SELECT_BY_TICKET)) {
	Print("Order info", 
			". symbol:", Symbol(),
			". ticket:", last_buy_ticket,
			". price width:", price_width,
			". lot:", lot,
			". profit lower:", profit_lower, 
			". profit unit:", profit_unit, 
			". base money:", base_money,
			". reverage:", AccountLeverage(),
			". accountCurrency:", AccountCurrency(),
			". account balance:", AccountBalance(),
			". account Free Margin:",AccountFreeMargin(),
			". account Free Margin Mode:",AccountFreeMarginMode()
			);
	} else {
    Print("OrderSelect returned the error of ",GetLastError());
	}
}
