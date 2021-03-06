# PyXchange #
Simulator of limit orderbook written in **Python** and **C++14**, using **Twisted** framework and **Boost** libraries, namely **boost::python** and **boost::multi_index** container.

The project is participating on *WOOD & Company Coding Challenge* [http://codingchallenge.wood.cz/]

## Main features ###

Matching engine implements simple limit orderbook with price/time priority matching algorithm.

Messaging protocol uses python dictionaries or JSON objects

The library can be used in two ways:

* as python package for scripting

* as a standalone TCP server

The TCP server is single thread, asynchronous, event driven. The server listens on three interfaces:

1. **Trading** interface for managing limit orders and market orders

2. **Extended trading** interface (**additional feature**: trader's orders are canceled, when the trader disconnects)

3. **Market data** interface, which is read-only

### Supported message types on trading interface ###

* `ping`/`pong` (heartbeat)

* `createOrder`

* `marketOrder`

* `cancelOrder`

* `cancelAll`

### Supported message types on market data interface ###

* `orderbook` (price level aggregated summary)
 
* `trade`


## Requirements ##

* Twisted framework

* Python 2.7 headers

* Boost libraries

* GCC with C++14 suport

* CMake

## Installation ##

### Build with CMake ###

```
$ mkdir build
$ cd build
$ cmake ..
$ make -j4
$ make install
```
### Creating `deb` package with CMake ###
```
$ make package 
$ dpkg -i ./pyxchange-0.1.0-Linux.deb
```

### Build with Distutils ###

```
$ python setup.py build
$ python setup.py install
```

### Creating `rpm` package ###
```
$ python setup.py bdist_rpm
$ rpm -ivh ./dist/pyxchange-0.1.0-1.x86_64.rpm
```


## Introduction to scripting API ##

### Creating `Matcher` instance
Matcher holds orderbook with orders. It also holds list of connected traders and clients as well.

```
>>> import logging
>>> logging.basicConfig(level=logging.DEBUG)
>>> from pyxchange import engine
>>> matcher = engine.Matcher()
INFO:pyxchange:OrderBook is ready
INFO:pyxchange:Matcher is ready
>>> matcher
<pyxchange.engine.Matcher object at 0x7f1be6ff2050>
>>>
```
### Creating `Trader` instance and sending heartbeat messages (ping/pong)
```
>>> from pyxchange import utils
>>> t1 = utils.DequeHandler()
>>> trader1 = engine.Trader(matcher, 'trader1', t1)
INFO:pyxchange:trader1 connected
>>> trader1
<pyxchange.engine.Trader object at 0x7f1be6ff20a8>
>>> trader1.ping()
>>> t1.messages.popleft()
{'message': 'pong'}
>>>
```
### Creating new order ###
OrderId must be unique within trader and bid/ask orders.

Side is `BUY` or `SELL`, price is positive integer, quantity is positive integer.
```
>>> trader1.createOrder({ 'side': 'BUY', 'price': 100, 'quantity': 10, 'orderId': 1 })
DEBUG:pyxchange:trader1 added order bid:10@100
>>> t1.messages.popleft()
{'report': 'NEW', 'orderId': 1, 'message': 'executionReport', 'quantity': 10}
```
### Triggering a match event ###
Trader2 creates sell order, which is matched with order of trader1.

Remaining quantity is inserted to the orderbook.

```
>>> t2 = utils.DequeHandler()
>>> trader2 = engine.Trader(matcher, 'trader2', t2)
INFO:pyxchange:trader2 connected
>>> trader2
<pyxchange.engine.Trader object at 0x7f1be50dc3c0>
>>> trader2.createOrder({ 'side': 'SELL', 'price': 90, 'quantity': 20, 'orderId': 1 })
DEBUG:pyxchange:Execution 10@100
DEBUG:pyxchange:trader2 added order ask:10@90
>>> t2.messages.popleft()
{'report': 'FILL', 'orderId': 1, 'message': 'executionReport', 'price': 100, 'quantity': 10}
>>> t2.messages.popleft()
{'report': 'NEW', 'orderId': 1, 'message': 'executionReport', 'quantity': 10}
>>>
```
### Canceling order ###
```
>>> trader2.cancelOrder({ 'side': 'SELL', 'orderId': 1 })
DEBUG:pyxchange:trader2 cancelled order ask:10@90
>>> t2.messages.popleft()
{'report': 'CANCELED', 'orderId': 1, 'message': 'executionReport', 'quantity': 10}
>>> trader1.cancelAll()
>>> 
```
### Creating market order ###
```
>>> trader1.createOrder({ 'side': 'BUY', 'price': 100, 'quantity': 10, 'orderId': 2 })
DEBUG:pyxchange:trader1 added order bid:10@100
>>> trader1.createOrder({ 'side': 'BUY', 'price': 150, 'quantity': 20, 'orderId': 3 })
DEBUG:pyxchange:trader1 added order bid:20@150
>>> trader2.marketOrder({ 'side': 'SELL', 'price': 150, 'quantity': 15 })
DEBUG:pyxchange:trader2 added market order ask:15
DEBUG:pyxchange:Execution 15@150
>>> 
```
### Connecting market-data client ###
New client receives complete price level aggregated data from actual orderbook.

Later it receives only updates on individual price levels and trade summaries.

In case more clients connected to matching engine, they will be notified in random order.
```
>>> c1 = utils.DequeHandler()
>>> client1 = engine.Client(matcher, 'client1', c1)
INFO:pyxchange:client1 connected
>>> client1
<pyxchange.engine.Client object at 0x7f1be50dc520>
>>> c1.messages
deque([{'price': 150, 'type': 'orderbook', 'side': 'bid', 'quantity': 5},
       {'price': 100, 'type': 'orderbook', 'side': 'bid', 'quantity': 10}])
>>>
```

## Introduction to standalone TCP server ##

### Running the server ###
```
$ pyxchange_server.py
2016-06-20 19:40:29,516 pyxchange INFO OrderBook is ready
2016-06-20 19:40:29,516 pyxchange INFO Matcher is ready
2016-06-20 19:40:29,518 pyxchange INFO Listeting on *:7000 (ext trading)
2016-06-20 19:40:29,518 pyxchange INFO Listeting on *:7001 (trading)
2016-06-20 19:40:29,519 pyxchange INFO Listeting on *:7002 (market-data)
```

### Connecting the trader ###

```
$ telnet localhost 7001
Trying ::1...
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
{"message": "ping"}
{"message": "pong"}
{"message": "createOrder", "side": "BUY", "price": 100, "quantity": 20, "orderId": 1 }
{"report": "NEW", "orderId": 1, "message": "executionReport", "quantity": 20}
```

### Connecting the client (market data) ###

```
$ telnet localhost 7002
Trying ::1...
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
{"price": 100, "type": "orderbook", "side": "bid", "quantity": 20}
```

### More options of the server ###

```
$ pyxchange_server.py --help
Usage: pyxchange_server.py [options]

Options:
  -h, --help            show this help message and exit
  --listen-private-ext=ip:port
                        Listen on extended private (trading) interface
  --listen-private=ip:port
                        Listen on private (trading) interface
  --listen-public=ip:port
                        Listen on public (market-data) interface
  --log=file            Log filename
  --format=format       Log format
  -d, --debug           Enable debug level
```

### Performance ###

Twisted -based server is very fast, however because the event-loop is single threaded, the performance is limited to the performance of single CPU.

The design of matching engine is trying to get the best from single CPU. The memory usage is as low as possibble.

On author's older workstation, matching engine is able to process 100k random `createOrder` messages in ~27 seconds (some of them trigger trades).

More traders and clients connected will naturally cause performance decrease of matching engine.

The performance test can be run from tests/ directory:
```
$ time python tests/performance_test.py -n 100000
    
real    0m26.724s
user    0m7.996s
sys     0m0.152s
```


## Implementation details ##

**Boost::python** is powerful framework for exposing C++ code to Python. The library exposes three classes to Python: Matcher, Trader and Client.

From Python side, instances of Matcher, Trader and Client are managed by shared pointers (`std::shared_ptr`).

Matcher controls creating of Clients, validation and dispatch of (JSON) messages.

OrderBook object within Matcher contains two **boost::multi_index** containers:

1. *BidOrderContainer* - greatest price first, lowest price last

2. *AskOrderContainer* - lowest price first, greatest price last

Both containers are very similar. They are indexed by multiple indexes:

1. *index by price-time*  (used for matching), orders are sorted by price, order at the same price level are sorted by time, lower priority first

2. *index by price*  (used for price level aggregation)

3. *index by trader* (used on `cancelAll` to find all orders of Trader)

4. *index by pair trader-orderId* (used on `createOrder` and `cancelOrder` to find order by ID)


Orders within BidOrderContainer and AskOrderContainer are managed by shared pointers and are considered as constant except the quantity, which may change during match event.

Order has weak pointer (`std::weak_ptr`) to Trader, so this basically allows to destroy the Trader, while Order continues to live (see also difference between trading and extended trading interface).


## Ideas for future enhancements ###

* Extended market data interface with order-by-order orderbook updates

* Administration interface for retrieving runtime performance statistics

* Support for `modifyOrder` message

* Support for `login`/`logout` messages

* Support for message sequence numbers 

* Self-match prevention

* Expose OrderBook, order containers and individual orders to Python

* Data persistence, pickle support


