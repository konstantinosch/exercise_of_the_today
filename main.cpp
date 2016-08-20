// Super Simple solution:
//
// Greetings!
//
// In this simplistic implementation you will find the following struct / classes / containers:
//
// a) SuperSimpleStock : A class that represents the stock as described in the definition with relevant methods.
// b) Trade : A simple collection of fields that represent the trade information to be recorded.
// c) SuperSimpleBroker : A class that represents the broker that can calculate stock price, all share index and record trades.
// d) GBCE_sample_stocks : A container of stocks from the given sample as an initial seed
// e) GBCE_exchange_stocks : A container that stores all stock price changes periodically
// f) SuperSimpleSimulator : A simple simulator with access to references of the above containers and SuperSimpleBroker that
//    simulates how the exchange container is populated and how the broker calculates and records trades using two seperated threads
//    across shared resources (containers, random engine, flags). The threads are the exchange_thread and broker_thread.
//
//     Comments regarding the threads are found in the scope of the functions that the threads execute later on.
//     I have kept everything in a single file on purpose and played around with C++11 for practice.
//
// Warm regards from an endless Greek summer,
//
//
// Konstantinos Chantzis

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <random>

#define FIFTEEN_MINUTES 9000 //ms set for testing
//#define EXCHANGE_DEBUG
//#define BROKER_STOCK_DEBUG
#define BROKER_TRADE_DEBUG

using namespace std;
using namespace std::chrono;

//======================================================================//
//======================================================================//

class SuperSimpleStock
{

public:
    SuperSimpleStock(string, string, double, double, double);
    double divident_yield();
    double PER();
    double get_par_value() const;
    void set_par_value(double);
    string get_stock_symbol();

    friend std::ostream& operator <<(std::ostream& o, const SuperSimpleStock& sss)
    {
        if (&sss == nullptr)
        {
            o << "[" << "NULL" << ":" << "NULL" << ":" << 0 << ":" << 0 << ":" << 0 << "]" << std::endl;
        }
        else
        {
            o << "[" << sss.stock_symbol_ << ":" << sss.stock_type_ << ":" << sss.last_divident_ << ":" << sss.fixed_divident_ << ":" << sss.par_value_ << "]" << std::endl;
        }
        return o;
    }


private:
    string stock_symbol_;
    string stock_type_;
    double last_divident_;
    double fixed_divident_;
    double par_value_;
};

SuperSimpleStock::SuperSimpleStock(string stock_symbol, string stock_type, double last_divident, double fixed_divident, double par_value) :
    stock_symbol_       (stock_symbol),
    stock_type_         (stock_type),
    last_divident_      (last_divident),
    fixed_divident_     (fixed_divident),
    par_value_          (par_value)
{}

double SuperSimpleStock::divident_yield()
{
    double dy=0;

    if ( par_value_ == 0 )
        return 0;

    dy = last_divident_ / par_value_;

    return dy;
}

double SuperSimpleStock::PER()
{
    double per=0;

    if ( last_divident_ == 0 )
        return 0;

    per = par_value_ / last_divident_;

    return per;
}

double SuperSimpleStock::get_par_value() const
{
    return par_value_;
}

void SuperSimpleStock::set_par_value(double pv)
{
    par_value_ = pv;
}

string SuperSimpleStock::get_stock_symbol()
{
    return stock_symbol_;
}

//======================================================================//
//======================================================================//

typedef struct Trade
{
    string stock_symbol;
    milliseconds timestamp;
    char bs_indicator;
    long quantity;
    double price;

    friend std::ostream& operator <<(std::ostream& o, const Trade& t)
    {
        if (&t == nullptr)
        {
            o << "[" << "NULL" << ":" << 0 << ":" << "NULL" << ":" << 0 << ":" << 0 << "]" << std::endl;
        }
        else
        {
            o << "[" << t.stock_symbol << ":" << t.timestamp.count() << ":" << t.bs_indicator << ":" << t.quantity << ":" << t.price << "]" << std::endl;
        }
        return o;
    }

} Trade;

//======================================================================//
//======================================================================//

class SuperSimpleBroker
{
    public:
        void record(const Trade&);
        double stock_price(string);
        double all_share_index(const vector<SuperSimpleStock>&);
    private:
        vector<Trade> trade_records_;
};

void SuperSimpleBroker::record(const Trade& t)
{
    trade_records_.push_back(t);
}

double SuperSimpleBroker::stock_price(string ss)
{
    double sp = 0; //stock price
    double sum_tp_q = 0; //sum of trade price and quantities product
    double sum_q = 0; //sum of quantities
    unsigned long long l_timestamp = 0; //last timestamp
    unsigned long long e_timestamp = 0; // earliest timestamp
    bool o_flag = false; //occurance flag

    for (vector<Trade>::reverse_iterator i = trade_records_.rbegin(); i != trade_records_.rend(); ++i)
    {
        if ( i->stock_symbol == ss )
        {
            if (o_flag == false)
            {
                l_timestamp = i->timestamp.count();
                o_flag = true;
            }
            else
            {
                e_timestamp = i->timestamp.count();
            }

            if (l_timestamp - e_timestamp >= FIFTEEN_MINUTES && l_timestamp !=0 && e_timestamp !=0 )
            {
                break;
            }

            sum_tp_q = (i->price * i->quantity) + sum_tp_q;
            sum_q = i->quantity + sum_q;
        }
    }

    if ( sum_q == 0 ) return 0;

    sp = sum_tp_q / sum_q;

    return sp;
}

double SuperSimpleBroker::all_share_index(const vector<SuperSimpleStock>& sssv)
{
    double asi = 0;

    long double sum = 0;

    for (vector<SuperSimpleStock>::const_iterator i = sssv.begin(); i != sssv.end(); ++i)
    {
        double stock_price = i->get_par_value();
        sum = log(stock_price) + sum;
    }

    asi = exp(sum/sssv.size()); //sum of logs

    return asi;
}

//======================================================================//
//======================================================================//

class SuperSimpleSimulator
{

public:
    SuperSimpleSimulator(const vector<SuperSimpleStock>&, vector<SuperSimpleStock>&, SuperSimpleBroker& );
    void start_sim();
    void stop_sim();

private:
    void brokers_life();
    void exchange_life();

    //random
    mt19937 rng_;

    //multitasking
    volatile bool life_;
    volatile bool exchange_thread_status_;
    volatile bool broker_thread_status_;
    mutex sim_mutex_;
    thread exchange_thread_;
    thread broker_thread_;

    //containers
    const vector<SuperSimpleStock>& sample_stocks_ref_;
    vector<SuperSimpleStock>& exchange_stocks_ref_;
    SuperSimpleBroker& broker_ref_;

};

SuperSimpleSimulator::SuperSimpleSimulator(const vector<SuperSimpleStock>& sr, vector<SuperSimpleStock>& gr, SuperSimpleBroker& br) :
    sample_stocks_ref_ (sr),
    exchange_stocks_ref_ (gr),
    broker_ref_ (br)
{
    rng_.seed(system_clock::now().time_since_epoch().count());
}

void SuperSimpleSimulator::brokers_life()
{
    std::cout << "SuperSimpleSimulator::brokers_life() ENTER" << endl;

    // The simple life of the broker in a nutshell:
    //
    // a) randomly pick a stock instance from samples,
    // b) create a trade instance and populate its members with some randomness
    // c) use the stock symbol to find the last live record
    //    of that stock from the exchange and copy its price to the trade ***
    // d) record the trade.
    // e) sleep for 2 seconds and repeat.

    while(life_)
    {
        sim_mutex_.lock();

        broker_thread_status_ = true;
        // a)

        if (sample_stocks_ref_.size() == 0 || exchange_stocks_ref_.size() == 0) return;

        SuperSimpleStock sss1 = sample_stocks_ref_.at(uniform_int_distribution<uint32_t>{0,sample_stocks_ref_.size()-1}(rng_));

        // b)

        Trade t;

        t.stock_symbol = sss1.get_stock_symbol();

        if (uniform_int_distribution<uint32_t>{0,1}(rng_))
        {
            t.bs_indicator = 'b';
        }
        else
        {
            t.bs_indicator = 's';
        }

        t.quantity = uniform_int_distribution<uint32_t>{0,50}(rng_);

        t.timestamp = duration_cast< milliseconds >(system_clock::now().time_since_epoch());

        // c)
        for (vector<SuperSimpleStock>::reverse_iterator i = exchange_stocks_ref_.rbegin(); i != exchange_stocks_ref_.rend(); ++i)
        {

            if (i->get_stock_symbol() == t.stock_symbol)
            {
                t.price = i->get_par_value();
                break;
            }
        }

        // d)
        broker_ref_.record(t);

        SuperSimpleStock sss2 = exchange_stocks_ref_.at(uniform_int_distribution<uint32_t>{0,exchange_stocks_ref_.size()-1}(rng_));

#ifdef BROKER_STOCK_DEBUG
        cout << endl;
        cout << sss2;
        cout << "divident yield: " << sss2.divident_yield() << endl;
        cout << "PER: " << sss2.PER() << endl;
        cout << endl;

        cout << endl;
        cout << "all share index: " << broker_ref_.all_share_index(exchange_stocks_ref_) << endl;
        cout << endl;
#endif

#ifdef BROKER_TRADE_DEBUG
        cout << endl;
        cout << "stock price: " << broker_ref_.stock_price(t.stock_symbol) << endl;
        cout << t << endl;
        cout << endl;
#endif

        sim_mutex_.unlock();

        // e)
        this_thread::sleep_for(seconds(3));
    }
    broker_thread_status_ = false;

    cout << "SuperSimpleSimulator::brokers_life() EXIT" << endl;
}

void SuperSimpleSimulator::exchange_life()
{
    cout << "SuperSimpleSimulator::exchange_life() ENTER" << endl;


    // The simple life of the exchange in a nutshell:
    //
    // a) randomly pick a stock instance from samples,
    // b) alter its price with a random uniform [0,30] +/- offset,
    // c) push it to the exchange container
    // d) sleep for 2 seconds and repeat.

    while(life_)
    {
        sim_mutex_.lock();
        exchange_thread_status_ = true;
        // a)
        if (sample_stocks_ref_.size() == 0) return;
        SuperSimpleStock sss = sample_stocks_ref_.at(uniform_int_distribution<uint32_t>{0,sample_stocks_ref_.size()-1}(rng_));

        // b)
        if (uniform_int_distribution<uint32_t>{0,2}(rng_))
        {
            sss.set_par_value(sss.get_par_value() + ( uniform_int_distribution<uint32_t>{0,30}(rng_) * sss.get_par_value() )/100 );
        }
        else
        {
            sss.set_par_value(sss.get_par_value() - ( uniform_int_distribution<uint32_t>{0,30}(rng_) * sss.get_par_value() )/100 );
        }

        // c)
        exchange_stocks_ref_.push_back(sss);

        sim_mutex_.unlock();

#ifdef EXCHANGE_DEBUG
        cout << sss;
#endif

        // d)
        this_thread::sleep_for(seconds(1));
    }
    exchange_thread_status_ = false;

    cout << "SuperSimpleSimulator::exchange_life() EXIT" << endl;
}

void SuperSimpleSimulator::start_sim()
{
    cout << "SuperSimpleSimulator::start_sim() ENTER" << endl;

    broker_thread_status_ = true;
    exchange_thread_status_ = true;
    life_ = true;

    exchange_thread_ = thread(&SuperSimpleSimulator::exchange_life,this);
    this_thread::sleep_for(seconds(5)); //give some time to populate the container
    broker_thread_ = thread(&SuperSimpleSimulator::brokers_life,this);

    cout << "SuperSimpleSimulator::start_sim() EXIT" << endl;
}

void SuperSimpleSimulator::stop_sim()
{
    life_ = false;

    while(exchange_thread_status_== true && broker_thread_status_ == true)
    {
        this_thread::sleep_for(seconds(1));
    }
    exchange_thread_.join();
    broker_thread_.join();
}

//======================================================================//
//======================================================================//

int main()
{
    //Sample data for easy generation
    vector<SuperSimpleStock> GBCE_sample_stocks;
    GBCE_sample_stocks.push_back(SuperSimpleStock("TEA","Common",0,0,100));
    GBCE_sample_stocks.push_back(SuperSimpleStock("POP","Common",8,0,100));
    GBCE_sample_stocks.push_back(SuperSimpleStock("ALE","Common",23,0,60));
    GBCE_sample_stocks.push_back(SuperSimpleStock("GIN","Preferred",8,0.2,100));
    GBCE_sample_stocks.push_back(SuperSimpleStock("JOE","Common",13,0,250));

    //The Global Beverage Corporation Exchange
    vector<SuperSimpleStock> GBCE_stocks;

    SuperSimpleBroker Konstantinos;
    SuperSimpleSimulator Sim(GBCE_sample_stocks, GBCE_stocks, Konstantinos);

    //start simulation
    Sim.start_sim();

    while(true)
    {
        this_thread::sleep_for(seconds(1000));

        //end simulation
        Sim.stop_sim();
        return 0;
    }
}








































