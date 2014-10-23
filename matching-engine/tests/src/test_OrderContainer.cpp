#include <gtest/gtest.h>

#include <vector>
#include <map>

#include <Engine_Order.h>
#include <Engine_OrderContainer.h>

#include <functional>

using namespace exchange::engine;

/**
    class DealHandler
*/
class DealHandler
{
    public:

        typedef std::map<UInt32, Deal*> DealContainerType;

    public:
        DealHandler():m_DealCounter(0)
        {}

        void OnDeal(Deal * ipDeal)
        {
            std::cout << "DealHandler : " << *ipDeal << std::endl;
            m_Deals.insert(std::make_pair(m_DealCounter, ipDeal));
            m_DealCounter++;
        }

        void Reset()
        {
            for (auto pDeal : m_Deals)
            {
                delete pDeal.second;
            }
            m_Deals.clear();
            m_DealCounter = 0;
        }

        const DealContainerType & GetDealContainer() const { return m_Deals; }

    private:

        UInt32                  m_DealCounter;
        DealContainerType       m_Deals;
};

// To use a test fixture, derive a class from testing::Test.
class OrderContainerTest : public testing::Test
{
    protected:

        typedef OrderContainer<Order, DealHandler>      OrderContainerType;
        typedef OrderContainerType::LimitContainer      LimitContainerType;
        typedef LimitContainerType::value_type          LimiteType;

    protected:
        OrderContainerTest():m_Container(m_DealHandler)
        {}

        void DisplayOrders()
        {
            m_Container.SetViewMode(OrderContainerType::VM_BY_ORDER);
            std::cout << m_Container << std::endl;

            m_Container.SetViewMode(OrderContainerType::VM_BY_PRICE);
            std::cout << m_Container << std::endl;
        }

        void InsertOrders()
        {
            for (auto & order : m_BidOrders)
            {
                ASSERT_TRUE(m_Container.Insert(order));
                ASSERT_FALSE(m_Container.Insert(order));
            }

            for (auto & order : m_AskOrders)
            {
                ASSERT_TRUE(m_Container.Insert(order));
                ASSERT_FALSE(m_Container.Insert(order));
            }
        }

        virtual void SetUp()
        {

            m_BidContainerReference = {
                                         LimiteType(2, 11000, 2185), LimiteType(1, 4000, 1325),
                                         LimiteType(1, 3000, 1321), LimiteType(2, 3000, 1234)
                                      };

            
            m_AskContainerReference = {
                                         LimiteType(2, 15000, 4321), LimiteType(1, 6000, 4526),
                                         LimiteType(1, 5000, 4580), LimiteType(2, 7000, 8526)
                                      };

            m_BidOrders = {
                             { BUY, 1000, 1234, 1, 5 }, { BUY, 2000, 1234, 1, 6 },
                             { BUY, 3000, 1321, 1, 7 }, { BUY, 4000, 1325, 1, 8 },
                             { BUY, 5000, 2185, 1, 9 }, { BUY, 6000, 2185, 1, 10 }
                          };

            m_AskOrders = {
                             { SELL, 8000, 4321, 2, 1 }, { SELL, 7000, 4321, 2, 2 },
                             { SELL, 6000, 4526, 2, 3 }, { SELL, 5000, 4580, 2, 4 },
                             { SELL, 4000, 8526, 2, 5 }, { SELL, 3000, 8526, 2, 6 }
                          };
        }

        
        OrderContainerType  m_Container;
        DealHandler         m_DealHandler;

        LimitContainerType  m_BidContainerReference;
        LimitContainerType  m_AskContainerReference;

        std::vector<Order>  m_BidOrders;
        std::vector<Order>  m_AskOrders;
};

TEST_F(OrderContainerTest, AuctionInsert)
{
    InsertOrders();

    DisplayOrders();

    OrderContainerType::LimitContainer BidContainer;
    OrderContainerType::LimitContainer AskContainer;

    m_Container.AggregatedView(BidContainer, AskContainer);

    ASSERT_TRUE(BidContainer == m_BidContainerReference);
    ASSERT_TRUE(AskContainer == m_AskContainerReference);
}

TEST_F(OrderContainerTest, AuctionDelete)
{
    InsertOrders();
    
    DisplayOrders();

    ASSERT_TRUE(m_Container.Delete(1, 5, BUY));
    ASSERT_FALSE(m_Container.Delete(1, 5, BUY));

    ASSERT_TRUE(m_Container.Delete(1, 10, BUY));

    ASSERT_TRUE(m_Container.Delete(2, 3, SELL));
    ASSERT_FALSE(m_Container.Delete(2, 3, SELL));

    ASSERT_TRUE(m_Container.Delete(2, 4, SELL));

    DisplayOrders();

    OrderContainerType::LimitContainer BidContainer;
    OrderContainerType::LimitContainer AskContainer;

    m_Container.AggregatedView(BidContainer, AskContainer);

    /*
    
    Transition from 

    |         BID          |         ASK         |
    |                      |                     |
    |  2   11000@2185      |  2   15000@4321     |
    |  1   4000@1325       |  1   6000@4526      |
    |  1   3000@1321       |  1   5000@4580      |
    |  2   3000@1234       |  2   7000@8526      |

    To :

    |         BID          |         ASK         |
    |                      |                     |
    |  1   5000@2185       |  2   15000@4321     |
    |  1   4000@1325       |  2   7000@8526      |
    |  1   3000@1321       |              0      |
    |  1   2000@1234       |              0      |
    
    */

    m_BidContainerReference = {
                                LimiteType(1, 5000, 2185), LimiteType(1, 4000, 1325),
                                LimiteType(1, 3000, 1321), LimiteType(1, 2000, 1234)
                              };


    m_AskContainerReference = {
                                LimiteType(2, 15000, 4321), LimiteType(2, 7000, 8526),
                              };

    ASSERT_TRUE(BidContainer == m_BidContainerReference);
    ASSERT_TRUE(AskContainer == m_AskContainerReference);
}

TEST_F(OrderContainerTest, AuctionModify)
{
    InsertOrders();

    DisplayOrders();

    /* OrderWay iWay, qty_type iQty, price_type iPrice, UInt32 iExistingOrderID, UInt32 iReplacedID, UInt32 iClientID */
    OrderReplace ReplaceBuy(BUY, 1337, 2185, 1, 2, 8);
    ASSERT_TRUE(m_Container.Modify(ReplaceBuy));

    OrderReplace ReplaceSell(SELL, 3000, 4526, 2, 12, 4);
    ASSERT_TRUE(m_Container.Modify(ReplaceSell));

    DisplayOrders();

    /*
        Transition from

        |         BID          |         ASK         |
        |                      |                     |
        |  2   11000@2185      |  2   15000@4321      |
        |  1   4000@1325       |  1   6000@4526      |
        |  1   3000@1321       |  1   5000@4580      |
        |  2   3000@1234       |  2   7000@8526      |

        To : 

        |         BID          |         ASK         |
        |                      |                     |
        |  3   12337@2185      |  2   15000@4321      |
        |  1   3000@1321       |  2   9000@4526      |
        |  2   3000@1234       |  2   7000@8526      |

    */

    OrderContainerType::LimitContainer BidContainer;
    OrderContainerType::LimitContainer AskContainer;

    m_Container.AggregatedView(BidContainer, AskContainer);

    m_BidContainerReference = {
                                 LimiteType(3, 12337, 2185), LimiteType(1, 3000, 1321),
                                 LimiteType(2, 3000, 1234)
                              };


    m_AskContainerReference = {
                                 LimiteType(2, 15000, 4321), LimiteType(2, 9000, 4526),
                                 LimiteType(2, 7000, 8526)
                              };

    ASSERT_TRUE(BidContainer == m_BidContainerReference);
    ASSERT_TRUE(AskContainer == m_AskContainerReference);

}

TEST_F(OrderContainerTest, AuctionFixing)
{
    m_Container.Reset();

    m_BidOrders = {
                     { BUY, 1200, 90, 1, 5 }, { BUY, 350, 89, 1, 6 },
                     { BUY, 150, 88, 1, 7 }, { BUY, 230, 87, 1, 8 }
                  };

    m_AskOrders = {
                     { SELL, 900, 90, 2, 1 }, { SELL, 650, 91, 2, 2 },
                     { SELL, 500, 92, 2, 3 }, { SELL, 350, 93, 2, 4 },
                     { SELL, 400, 94, 2, 5 }
                  };

    InsertOrders();

    auto OpenPrice = m_Container.GetTheoriticalOpenInformations();

    ASSERT_EQ(std::get<0>(OpenPrice), 90);
    ASSERT_EQ(std::get<1>(OpenPrice), 900);
    
    DisplayOrders();

    m_Container.Reset();


    m_BidOrders = {
                     { BUY, 200, 41, 1, 5 }, { BUY, 300, 40, 1, 6 },
                     { BUY, 150, 39, 1, 7 }, { BUY, 50, 38, 1, 8 },
                     { BUY, 10, 37, 1, 9 }
                  };

    m_AskOrders = {
                     { SELL, 100, 35, 2, 1 }, { SELL, 200, 36, 2, 2 },
                     { SELL, 50, 37, 2, 3 }, { SELL, 200, 39, 2, 4 },
                     { SELL, 20, 40, 2, 5 }
                  };

    InsertOrders();

    DisplayOrders();

    OpenPrice = m_Container.GetTheoriticalOpenInformations();

    ASSERT_EQ(std::get<0>(OpenPrice), 39);
    ASSERT_EQ(std::get<1>(OpenPrice), 550);
}

TEST_F(OrderContainerTest, AuctionMatching)
{
    m_Container.Reset();

    m_BidOrders = {
                      { BUY, 1200, 90, 1, 5 }, { BUY, 350, 89, 1, 6 },
                      { BUY, 150, 88, 1, 7 }, { BUY, 230, 87, 1, 8 }
                  };

    m_AskOrders = {
                     { SELL, 900, 90, 2, 1 }, { SELL, 650, 91, 2, 2 },
                     { SELL, 500, 92, 2, 3 }, { SELL, 350, 93, 2, 4 },
                     { SELL, 400, 94, 2, 5 }
                  };

    InsertOrders();
    
    DisplayOrders();

    m_Container.MatchOrders();

    auto & DealContainer = m_DealHandler.GetDealContainer();

    ASSERT_EQ(DealContainer.size(), 1);

    ASSERT_EQ( *DealContainer.at(0) , Deal(90, 900, 1, 2, 5, 1) );

    DisplayOrders();

    /*
        
    */
    m_DealHandler.Reset();
    m_Container.Reset();

    m_BidOrders = {
                     { BUY, 200, 41, 1, 5 }, { BUY, 300, 40, 1, 6 },
                     { BUY, 150, 39, 1, 7 }, { BUY, 50, 38, 1, 8 },
                     { BUY, 10, 37, 1, 9 }
                  };

    m_AskOrders = {
                     { SELL, 100, 35, 2, 1 }, { SELL, 200, 36, 2, 2 },
                     { SELL, 50, 37, 2, 3 }, { SELL, 200, 39, 2, 4 },
                     { SELL, 20, 40, 2, 5 }
                  };

    InsertOrders();
    DisplayOrders();

    m_Container.MatchOrders();

    ASSERT_EQ(DealContainer.size(), 6);

    ASSERT_EQ(*DealContainer.at(0), Deal(39, 100, 1, 2, 5, 1));
    ASSERT_EQ(*DealContainer.at(1), Deal(39, 100, 2, 2, 5, 1));
    ASSERT_EQ(*DealContainer.at(2), Deal(39, 100, 2, 2, 6, 1));
    ASSERT_EQ(*DealContainer.at(3), Deal(39, 50, 3, 2, 6, 1));
    ASSERT_EQ(*DealContainer.at(4), Deal(39, 150, 4, 2, 6, 1));
    ASSERT_EQ(*DealContainer.at(5), Deal(39, 50, 4, 2, 7, 1));

    DisplayOrders();

    m_DealHandler.Reset();
    m_Container.Reset();
}

TEST_F(OrderContainerTest, InsertMatching)
{
    auto & DealContainer = m_DealHandler.GetDealContainer();

    m_Container.Reset();

    m_BidOrders = {
                     { BUY, 350, 89, 1, 6 }, { BUY, 150, 88, 1, 7 },
                     { BUY, 230, 87, 1, 8 }
                  };

    m_AskOrders = {
                    { SELL, 900, 90, 2, 1 }, { SELL, 650, 91, 2, 2 },
                    { SELL, 500, 92, 2, 3 }, { SELL, 350, 93, 2, 4 }
                  };

    InsertOrders();

    DisplayOrders();

    Order BuyOrder(BUY, 123, 88, 5, 1);
    Order SellOrder(SELL, 123, 91, 5, 1);

    ASSERT_TRUE(m_Container.Insert(BuyOrder, true));
    ASSERT_TRUE(m_Container.Insert(SellOrder, true));

    DisplayOrders();

    ASSERT_EQ(DealContainer.size(), 0);

    BuyOrder   = Order(BUY, 500, 91, 6, 1);
    SellOrder  = Order(SELL, 150, 89, 6, 1);

    ASSERT_TRUE(m_Container.Insert(BuyOrder, true));
    ASSERT_TRUE(m_Container.Insert(SellOrder, true));

    ASSERT_EQ(DealContainer.size(), 2);
    
    DisplayOrders();

    BuyOrder = Order(BUY, 2500, 93, 10, 1);
    SellOrder = Order(SELL, 1500, 87, 10, 1);

    ASSERT_TRUE(m_Container.Insert(BuyOrder, true));

    DisplayOrders();

    ASSERT_TRUE(m_Container.Insert(SellOrder, true));

    DisplayOrders();


    m_BidContainerReference = {};

    m_AskContainerReference = { LimiteType(1, 320, 87) };

    OrderContainerType::LimitContainer BidContainer;
    OrderContainerType::LimitContainer AskContainer;

    m_Container.AggregatedView(BidContainer, AskContainer);

    ASSERT_TRUE(BidContainer == m_BidContainerReference);
    ASSERT_TRUE(AskContainer == m_AskContainerReference);

    m_DealHandler.Reset();
}

TEST_F(OrderContainerTest, ModifyMatching)
{
    m_Container.Reset();

    m_BidOrders = {
                     { BUY, 350, 89, 1, 6 }, { BUY, 150, 88, 1, 7 },
                     { BUY, 230, 87, 1, 8 }
                  };

    m_AskOrders = {
                     { SELL, 900, 90, 2, 1 }, { SELL, 650, 91, 2, 2 },
                     { SELL, 500, 92, 2, 3 }, { SELL, 350, 93, 2, 4 }
                  };

    InsertOrders();

    DisplayOrders();

    OrderReplace BuyReplace(BUY, 500, 88, 1, 2, 6);
    OrderReplace SellReplace(SELL, 1200, 91, 2, 4, 1);


    ASSERT_TRUE(m_Container.Modify(BuyReplace, true));

    DisplayOrders();

    ASSERT_TRUE(m_Container.Modify(SellReplace, true));

    DisplayOrders();
    
    BuyReplace = OrderReplace(BUY, 2000, 91, 1, 5, 8);

    ASSERT_TRUE(m_Container.Modify(BuyReplace, true));

    DisplayOrders();

    SellReplace = OrderReplace(SELL, 500, 91, 2, 8, 4);

    ASSERT_TRUE(m_Container.Modify(SellReplace, true));

    DisplayOrders();

    m_BidContainerReference = { LimiteType(2, 650, 88) };

    m_AskContainerReference = { LimiteType(1, 350, 91), LimiteType(1, 500, 92) };

    OrderContainerType::LimitContainer BidContainer;
    OrderContainerType::LimitContainer AskContainer;

    m_Container.AggregatedView(BidContainer, AskContainer);

    ASSERT_TRUE(BidContainer == m_BidContainerReference);
    ASSERT_TRUE(AskContainer == m_AskContainerReference);

    // TODO : Add a check for all deals generated
    m_DealHandler.Reset();
}


int main(int argc, char ** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}