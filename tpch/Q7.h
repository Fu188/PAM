using pfloat = array<double, 50>;
using Q7_rtype = array<pfloat, 2>;

Q7_rtype Q7(maps m, size_t n1id, size_t n2id, const char* _start, const char* _end) {
    Date start = Date(_start);
    Date end = Date(_end);

    using Add = Add_Nested_Array<Q7_rtype>;
    auto customer_f = [&](Q7_rtype &a, customer_map::E &ce) -> void{
        unsigned int customer_nation = ce.second.first.c_n_nationkey;
        order_map &omap = ce.second.second;
        if (customer_nation == n2id || customer_nation == n1id) {
            auto order_f = [&](order_map::E &oe) {
                Order &o = oe.second.first;
                int entry_year = o.o_entry_d.year();
                ol_map &ol = oe.second.second;
                auto ol_f = [&](OrderLine &ol) {
                    maybe<Stock> stock = static_data.ism.find(make_pair(ol.ol_i_id, ol.ol_supply_w_id));
                    if (!stock) return 0.0;
                    Supplier &supp = static_data.all_supp[(*stock).s_su_suppkey];
                    Date delivery_date = ol.ol_delivery_d;
                    double rev = ol.ol_amount;
                    int year = delivery_date.year();
                    if (Date::greater(delivery_date, start) && Date::less(delivery_date, end)) {
                        if (customer_nation == n2id && supp.su_nationkey == n1id) {
                            a[1][entry_year-1980] += rev;
                        }else if (customer_nation == n1id && supp.su_nationkey == n2id) {
                            a[0][entry_year-1980] += rev;
                        }
                    }
                };
                ol_map::foreach_seq(ol, ol_f);
            };
            order_map::foreach_seq(omap, order_f);
        }
    };
    return customer_map::semi_map_reduce(m.cm, customer_f, Add(), 4000);
}

double Q7time(maps m, bool verbose) {
    timer t;
    t.start();
    unsigned int n1id = 55;
    unsigned int n2id = 115;
    const char start[] = "2007-01-02";
    const char end[] = "2012-01-02";

    Q7_rtype result = Q7(m, n1id, n2id, start, end);

    double ret_tm = t.stop();
    cout << "Q7 : " << ret_tm << endl;

    if (verbose) {
        for (int i=0; i<50; i++){
            if (result[0][i] != 0 || result[1][i] !=0) {
                cout << "Q7:" << endl
                     << static_data.all_nation[n1id].name() << ", "
                     << i + 1980<< ", "
                     << result[0][i] << endl;
                cout << "Q7:" << endl
                     << static_data.all_nation[n2id].name() << ", "
                     << i + 1980<< ", "
                     << result[1][i] << endl;
            }
        }
    }
    return ret_tm;
}
