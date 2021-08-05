#include "pbbslib/strings/string_basics.h"
#include <string.h>

#include <set>

double quantity = 0;
double amount = 0;
double cnt = 0;

void copy_string(char *dest, char *src, size_t max_len) {
    if (strlen(src) > max_len) {
        std::cout << "string too long while parsing tables" << std::endl;
        abort();
    }
    strcpy(dest, src);
}

int str_to_int(char *str) {
    return atoi(str);
}

double str_to_double(char *str) {
    return atof(str);
}

using uint = unsigned int;
using ushort = unsigned short;
using uchar = unsigned char;

struct Date {
    union as {
        uint c;
        struct {
            ushort day;
            ushort month;
            ushort year;
        } d;
    };
    as v;
    const static int start_year = 0;

    static bool less(Date a, Date b) {
        return a.v.c < b.v.c;
    }

    static bool less_or_equal(Date a, Date b) {
        return a.v.c <= b.v.c;
    }

    static bool greater(Date a, Date b) {
        return a.v.c > b.v.c;
    }

    static bool greater_or_equal(Date a, Date b) {
        return a.v.c >= b.v.c;
    }

    int year() { return v.d.year + start_year; }

    int month() { return v.d.month; }

    int day() { return v.d.day; }

    void from_str(const char *str) {
        char str_cp[11];
        strcpy(str_cp, str);
        str_cp[4] = '\0';
        str_cp[7] = '\0';
        str_cp[10] = '\0';
        v.d.year = str_to_int(str_cp) - start_year;
        v.d.month = str_to_int(str_cp + 5);
        v.d.day = str_to_int(str_cp + 8);
    }

    friend ostream &operator<<(ostream &stream, const Date &date) {
        stream << ((uint) date.v.d.year + start_year) << "-"
               << ((uint) date.v.d.month) << "-" << date.v.d.day;
        return stream;
    }

    void add_one_day() {
        v.d.day++;
        if (v.d.day > 28) {
            v.d.month++;
            v.d.day = 1;
            if (v.d.month > 12) {
                v.d.year++;
                v.d.month = 1;
            }
        }
    }

    Date() = default;

    Date(int day, int month, int year) {
        v.d.year = year - start_year;
        v.d.month = month;
        v.d.day = day;
    }

    Date(const string x) { from_str(x.c_str()); }

    Date(char *str) { from_str(str); }

    string to_str() {
        string y = std::to_string(year());
        string m = std::to_string(month());
        if (month() < 10) m = "0" + m;
        string d = std::to_string(day());
        if (day() < 10) d = "0" + d;
        string result = y + "-" + m + "-" + d;
        return result;
    }

};

const static Date infty_date = Date(0, 0, 2100);

using dkey_t = uint;
using linenumber_t = uchar;
using quantity_t = uchar;
using price_t = float;

block_allocator customer_str_pool(58);

// size 32
struct Customer {
    uint c_pkey;
    uint c_id;
    uint c_d_id;
    uint c_w_id;
    float c_balance;
    uint c_n_nationkey;
    char *strings;

    Customer() {}

    Customer(sequence<char *> const &S) {
        c_pkey = str_to_int(S[22]);
        c_id = str_to_int(S[0]);
        c_d_id = str_to_int(S[1]);
        c_w_id = str_to_int(S[2]);
        c_balance = atof(S[16]);
        c_n_nationkey = str_to_int(S[21]);
        strings = (char*) customer_str_pool.alloc();

        copy_string(strings, S[5], 16);
        copy_string(strings + 17, S[8], 20);
        copy_string(strings + 38, S[9], 2);
        copy_string(strings + 41, S[11], 16);
    }

    char *c_last() {return strings;}
    char *c_city() {return strings + 17;}
    char *c_state() {return strings + 38;}
    char *c_phone() {return strings + 41;}
};

// size 28
struct Order {
    uint o_id;
    uint o_d_id;
    uint o_w_id;
    uint o_c_id;
    Date o_entry_d;
    uint o_carrier_id;
    uint o_ol_cnt;
    uint o_pkey;
    uint o_c_pkey;

    Order() {}

    Order(sequence<char *> const &S) {
        o_id = str_to_int(S[0]);
        o_d_id = str_to_int(S[1]);
        o_w_id = str_to_int(S[2]);
        o_c_id = str_to_int(S[3]);
        o_entry_d = Date(S[4]);
        o_carrier_id = str_to_int(S[5]);
        o_ol_cnt = str_to_int(S[6]);
        o_pkey = str_to_int(S[8]);
        o_c_pkey = str_to_int(S[9]);
    }
};

block_allocator item_str_pool(76);

// size 16
struct Item {
    uint i_id;
    float i_price;
    char *strings;

    Item() {}

    Item(sequence<char *> const &S) {
        i_id = str_to_int(S[0]);
        i_price = atof(S[3]);
        strings = (char *) item_str_pool.alloc();

        copy_string(strings, S[2], 24);
        copy_string(strings + 25, S[4], 50);
    }

    char *i_name() {return strings;}
    char *i_data() {return strings + 25;}
};

// size 20
struct Stock{
    uint s_i_id;
    uint s_w_id;
    uint s_quantity;
    uint s_order_cnt;
    uint s_su_suppkey;

    Stock() {}

    Stock(sequence<char *> const &S) {
        s_i_id = str_to_int(S[0]);
        s_w_id = str_to_int(S[1]);
        s_quantity = str_to_int(S[2]);
        s_order_cnt = str_to_int(S[14]);
        s_su_suppkey = str_to_int(S[17]);
    }
};

// size 36
struct OrderLine {
    uint ol_pkey;
    uint ol_o_id;
    uint ol_d_id;
    uint ol_w_id;
    uint ol_number;
    uint ol_i_id;
    uint ol_supply_w_id;
    Date ol_delivery_d;
    uint ol_quantity;
    uint ol_suppkey;
    float ol_amount;

    OrderLine() = default;

    OrderLine(sequence<char *> const &S) {
        ol_o_id = str_to_int(S[0]);
        ol_d_id = str_to_int(S[1]);
        ol_w_id = str_to_int(S[2]);
        ol_number = str_to_int(S[3]);
        ol_i_id = str_to_int(S[4]);
        ol_supply_w_id = str_to_int(S[5]);
        ol_delivery_d = Date(S[6]);
        ol_quantity = str_to_int(S[7]);
        ol_amount = atof(S[8]);
        ol_pkey = str_to_int(S[10]);
        ol_suppkey = (ol_i_id*ol_w_id)%10000;
    }
};

// size 16
struct Nation {
    uint n_nationkey;
    uint n_regionkey;
    char* n_name;

    Nation() {}

    Nation(sequence<char *> const &S) {
        n_nationkey = str_to_int(S[0]);
        n_name = new char[strlen(S[1]) + 1];
        strcpy(n_name, S[1]);
        n_regionkey = str_to_int(S[2]);
    }

    char *name() {return n_name;}
};

sequence<Nation> nations;

block_allocator supp_str_pool(185);

// size 16
struct Supplier {
    uint su_suppkey;
    uint su_nationkey;
    char *strings;

    Supplier() {}

    Supplier(sequence<char *> const &S) {
        su_suppkey = str_to_int(S[0]);
        su_nationkey = str_to_int(S[3]);
        strings = (char *) supp_str_pool.alloc();

        copy_string(strings, S[1], 25); // name
        copy_string(strings + 26, S[4], 15); //phone
        copy_string(strings + 42, S[2], 40); //address
        copy_string(strings + 83, S[6], 101); //address
    }

    char *su_name() { return strings; } // 25 chars, S[1]
    char *su_phone() { return strings + 26; } // 15 digits, S[4]
    char *su_address() { return strings + 42; } // 40 chars, S[2]
    char *su_comment() { return strings + 83; } //101 chars, S[6]
};





	
