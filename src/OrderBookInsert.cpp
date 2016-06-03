/**
 * @brief This module implements simulator of exchange
 * @file OrderBookInsert.cpp
 *
 */


#include "OrderBook.hpp"
#include "Constants.hpp"
#include "Trader.hpp"
#include "Side.hpp"


namespace pyxchange
{

namespace py = boost::python;


/**
 * @brief FIXME
 *
 */
template<typename OrderContainer, typename OppOrderContainer>
void OrderBook::insertOrder( typename OrderContainer::type& orders,
                             typename OppOrderContainer::type& oppOrders,
                             const TraderPtr& trader, const OrderPtr& order )
{
    const auto& result = orders.template insert( order );

    if( result.second )
    {
        if( handleSelfMatch<OppOrderContainer>( oppOrders, trader, order ) )
        {
            handleExecution<OppOrderContainer>( oppOrders, trader, order );

            if( order->quantity > 0 )
            {
                trader->notifyCreateOrderSuccess( order->orderId );

                aggregatePriceLevel<OrderContainer>( orders, order->price, order->side );

                logger.info( boost::format( format::traderAddedOrder ) % trader->getName()
                             % side::toBidAsk( order->side ) % order->orderId % order->price % order->quantity );
            }
            else
            {
                // order has no resting quantity (was executed), so remove it
                orders.template erase( result.first );
            }
        }
        else
        {
            // self-match prevention, erase order
            orders.template erase( result.first );
        }
    }
    else
    {
        logger.warning( boost::format( format::logOrderAlreadyExist ) % trader->getName() % order->orderId );

        trader->notifyOrderAlreadyExist( order->orderId );
    }
}


template void OrderBook::insertOrder<BidOrderContainer, AskOrderContainer>(
    BidOrderContainer::type& orders, AskOrderContainer::type& oppOrders,
    const TraderPtr& trader, const OrderPtr& order );

template void OrderBook::insertOrder<AskOrderContainer, BidOrderContainer>(
    AskOrderContainer::type& orders, BidOrderContainer::type& oppOrders,
    const TraderPtr& trader, const OrderPtr& order );


} /* namespace pyxchange */



/* EOF */

