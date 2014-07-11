#include <iostream>
#include <fstream>
#include <list>
#include <cstdlib>
#include <cmath>
using namespace std;

#ifndef REQ_H
#define REQ_H

#define MAX 3

class Req
{
    public:
        long long id;  //request id
        int last_time; /*处理持续时间周期*/
        Req(long long i);
        virtual ~Req();
    protected:
    private:
};


Req::Req(long long i)
{
    //ctor
    this->id = i;
    this->last_time = rand()%MAX+1;
}

Req::~Req()
{
    //dtor
}

#endif // REQ_H

ofstream fout1,fout2,fout3;


//Global Variables

list<Req> producer_list;
list<Req> private_list;
list<Req> public_list;

long long i=0;

int private_rate = 1200;  //私有云处理能力，每秒最多处理多少个request
int private_current;
int public_rate = 100;   //公有云处理能力，每秒最多处理多少个request
int public_current;
int pipe=1000;          //管道容量

const int T=100;       //一个VM的处理能力,一个回合request的数量
const float C = 0.05;          //公有云上一个VM在每一个回合的支出
double total_cost = 0;

void producer()
{
    i++;
    Req req(i);
    producer_list.push_back(req);

}

/*在一个回合结束后清空发送队列中没有处理的request，
事实上是在request达到loadbalancer的时候就过滤掉超过处理能力的部分*/

void drop_out()
{
    fout1 << producer_list.size() <<endl;
    if(!producer_list.empty())
        producer_list.clear();
}

void private_consumer()
{
    list<Req>::iterator it;
    for(it = private_list.begin();it != private_list.end(); )
    {
        it->last_time--;
        if(it->last_time == 0)
            it=private_list.erase(it);
        else it++;
    }
    private_current = private_list.size();
}

void public_comsumer()
{
    list<Req>::iterator it;
    for(it = public_list.begin();it != public_list.end();)
    {
        it->last_time--;

        if(it->last_time == 0)
            it = public_list.erase(it);
        else it++;
    }

    public_current = public_list.size();
    //cout<<public_current<<endl;
}

void loadbalancer1()
{
    int i=0;
    while(i++ < private_rate - private_current && !producer_list.empty())
    {
        Req req = producer_list.front();
        producer_list.pop_front();
        private_list.push_back(req);
    }
    private_current = private_list.size();

    i=0;
    total_cost += (public_rate/T) * C;

    while(i++ < public_rate - public_current && i < pipe && !producer_list.empty())
    {
        Req req = producer_list.front();
        producer_list.pop_front();
        public_list.push_back(req);
    }
    public_current = public_list.size();

    if(!producer_list.empty() && i < pipe)
    {
        int k = producer_list.size()/T + 1;
        public_rate += k*T;
        //total_cost += (public_rate/T) * C;
    }
    else{
        if(public_rate - public_current > T)
        {
            int h = (public_rate - public_current)/T ;
            public_rate -= h*T;

        }
    }
    cout << " " <<private_current<<" "<<public_current<<" ";
    cout << " " <<private_rate<<" "<<public_rate<<" ";
}


void loadbalancer2()
{
    int i=0;
    int S=producer_list.size();

    total_cost += (public_rate/T) * C;  //记录收费
    int trans = 0;
    int rest=0; //当公有云满载时，produce链上的个数

    while(i < S)
    {
        Req req = producer_list.front();
        producer_list.pop_front();
        int flag = rand()%2;
        if(flag && private_list.size() <= private_rate )
        {
            private_list.push_back(req);
            i++;
        }
        else
            if(!flag && public_list.size() <= public_rate && trans++ < pipe)
            {
                public_list.push_back(req);
                i++;

                if(public_list.size() >= public_rate)
                    rest = producer_list.size();
            }
            else
                producer_list.push_back(req);
        if((public_list.size() >= public_rate || trans >=pipe )&& private_list.size() >= private_rate )
            break;

    }

    private_current = private_list.size();
    public_current = public_list.size();

    if(rest>0)
    {
        int k = rest/T + 1;
        public_rate += k*T;
    }
    else{
        if(public_rate - public_current > T)
        {
            int h = (public_rate - public_current)/T ;
            public_rate -= h*T;

        }
    }
    cout << " " <<private_current<<" "<<public_current<<" ";
    cout << " " <<private_rate<<" "<<public_rate<<" ";
}

#define PI 3.1415926
int normal_distribution(int u,int k)
{
    return int((5000.0/sqrt(2*PI))*exp(0-(k-u)*(k-u)/30000.0));
}

int main()
{
    int rounds = 1000;
    int come_in;
    fout1.open("drop_out2.txt");
    fout2.open("cost2.txt");
    fout3.open("request2.txt");

    srand(13);

    for(int k=1; k <=rounds; k++)
    {
        come_in = normal_distribution(rounds/2,k);
        cout<<k<<' '<<come_in<<" ";
        fout3<<come_in<<endl;

        //come_in = rand()%500+500;

        while(come_in-- > 0)
            producer();

        loadbalancer2();

        private_consumer();

        public_comsumer();

        drop_out();

        cout<<endl;
    }
    fout2<<total_cost<<endl;

    fout1.close();
    fout2.close();
    fout3.close();

    return 0;
}
