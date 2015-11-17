#property strict

double price_width = 0.2;
double lot = 0.01;
bool first_order = TRUE;
int last_buy_ticket = -1;
int last_sell_ticket = -1;
double last_buy_price ;
double last_sell_price ;
//tuka tani de henko suru
//USD
//int base_money = 20000;
//JP
int base_money = 2000000;
double fraction_range = 0;
double profit_lower = 60

//+------------------------------------------------------------------+
//| Expert initialization function                                   |
//+------------------------------------------------------------------+
int OnInit()
  {
   return(INIT_SUCCEEDED);
  }
//+------------------------------------------------------------------+
//| Expert deinitialization function                                 |
//+------------------------------------------------------------------+
void OnDeinit(const int reason)
  {
//--- destroy timer
   EventKillTimer();
      
  }
//+------------------------------------------------------------------+
//| Expert tick function                                             |
//+------------------------------------------------------------------+
void OnTick() {
	calcLot();

	if (first_order) {
	  last_buy_ticket = OrderSend(Symbol(), OP_BUY, lot, Ask, 3, 0, 0, "Buy", 10, 0, Blue);
		if (OrderSelect(last_buy_ticket, SELECT_BY_TICKET, MODE_TRADES)){
			last_buy_price = OrderOpenPrice();
			last_sell_price = last_buy_price;
		} else {
			Print("first order error. CODE:", GetLastError());
			return;
		}

		first_order = FALSE;
		return;
	}


	RefreshRates();
	if ((last_buy_price + price_width) <= Ask || Bid <= (last_sell_price - price_width)) {
		double lot_correction = 0;
		if(0 < AccountFreeMarginCheck(Symbol(), OP_BUY, lot) ) {
			int price_level = Ask * 10;
			if ((price_level % 10 ) < fraction_range) {
				lot_correction = 0.01;
			}
			last_buy_ticket = OrderSend(Symbol(), OP_BUY, lot + lot_correction, Ask, 3, 0, 0, "Buy", 10, 0, Blue);
			if (OrderSelect(last_buy_ticket, SELECT_BY_TICKET, MODE_TRADES)) {
				last_buy_price = OrderOpenPrice();
			} else {
				printInfo();
			}
		}

		int price_level = Bid * 10;
		if ((price_level % 10 ) < fraction_range) {
			lot_correction = 0.01;
		} else {
			lot_correction = 0;
		}
		if(0 < AccountFreeMarginCheck(Symbol(), OP_SELL, lot) ) {
		last_sell_ticket = OrderSend(Symbol(), OP_SELL, lot + lot_correction, Bid, 3, 0, 0, "Sell", 10, 0, Red);
			if (OrderSelect(last_sell_ticket, SELECT_BY_TICKET, MODE_TRADES)) {
				last_sell_price = OrderOpenPrice();
			} else {
				printInfo();
			}
		}

		payingClose();
	}

		return;	
}

void calcLot() {
	int f_margin = AccountBalance();
	if (f_margin < base_money) {
		lot = 0.01;
		double tmp = base_money / f_margin;
		tmp = MathRound(tmp);
		price_width = tmp/10;
	} else {
		price_width = 0.1;
		double tmp = f_margin / base_money;
		int fraction = f_margin % base_money;
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
