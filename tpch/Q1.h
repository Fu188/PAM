// 6 possible ol_number, 3 values each
constexpr int num_rows = 16;
constexpr int num_vals = 3;
using Q1_row = array<double, num_vals>;
using Q1_rtype = array<Q1_row, num_rows>;

Q1_rtype Q1(maps m, const char *end_date) {
    delivery_map delivery_range = delivery_map::downTo(m.dm, Date(end_date));
//    cout << delivery_range[0] << endl;
    //cout << "Q1: m.sm ref count: " << m.sm.root->ref_cnt << endl;
    using Add = Add_Nested_Array<Q1_rtype>;

    auto date_f = [](delivery_map::E &e) -> Q1_rtype {
        auto ol_f = [](Q1_rtype &a, OrderLine &ol) {
            int row = ol.ol_number;
            double quantity = ol.ol_quantity;
            double amount = ol.ol_amount;
//            cout << row << ", " << quantity << ", " <<  amount << endl;
            a[row][0] += quantity;
            a[row][1] += amount;
            a[row][2] += 1.0;
        };
        return ol_map::semi_map_reduce(e.second, ol_f, Add(), 2000);
    };
    return delivery_map::map_reduce(delivery_range, date_f, Add(), 1);
}

double Q1time(maps m, bool verbose) {
    timer t;
    t.start();
    const char end[] = "2007-01-02";

    Q1_rtype result = Q1(m, end);

    double ret_tm = t.stop();
    cout << "Q1 : " << ret_tm << endl;

    if (verbose) {
        cout << "Q1:" << endl;
        int i = 1;
        while(i<=15) {
            double quantity = result[i][0];
            double amount = result[i][1];
            double count = result[i][2];
            cout << i << ", "
                 << quantity << ", "
                 << amount << ", "
                 << quantity / count << ", "
                 << amount / count << ", "
                 << (size_t) count
                 << endl;
            ++i;
        }
    }
    return ret_tm;
}

