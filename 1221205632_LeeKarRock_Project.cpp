// TDS4223 Individual Project: Badminton Court Booking System
// Student ID: 1221205632
// Student Name: Lee Kar Rock
// Portable C++98. No external libraries or STL containers are used.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdio>
#include <cerrno>
using namespace std;

const int OPEN_MINUTES = 8 * 60;
const int CLOSE_MINUTES = 22 * 60;
const double HOURLY_RATE = 20.0; // Example project rate in RM per hour.

// ---------- Shared input and file helpers ----------
string trim(const string& s)
{
    string::size_type a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    string::size_type b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

bool validText(const string& s)
{
    if (s.empty() || s.size() > 80) return false;
    for (string::size_type i = 0; i < s.size(); ++i)
        if (s[i] == '|' || static_cast<unsigned char>(s[i]) < 32)
            return false;
    return true;
}

bool validID(const string& s)
{
    if (s.empty() || s.size() > 20) return false;
    for (string::size_type i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    }
    return true;
}

bool readText(const string& prompt, string& value, bool id = false)
{
    while (true)
    {
        cout << prompt;
        if (!getline(cin, value)) return false;
        value = trim(value);
        if ((id && validID(value)) || (!id && validText(value))) return true;
        cout << (id ? "Use 1-20 letters, digits, hyphens or underscores.\n" :
                        "Enter 1-80 characters without the | symbol.\n");
    }
}

bool parseInt(const string& text, int& value)
{
    istringstream input(text);
    char extra;
    return (input >> value) && !(input >> extra);
}

bool readInt(const string& prompt, int& value, int low, int high)
{
    string text;
    while (true)
    {
        cout << prompt;
        if (!getline(cin, text)) return false;
        if (parseInt(trim(text), value) && value >= low && value <= high)
            return true;
        cout << "Enter a whole number from " << low << " to " << high << ".\n";
    }
}

bool allDigits(const string& s)
{
    for (string::size_type i = 0; i < s.size(); ++i)
        if (s[i] < '0' || s[i] > '9') return false;
    return !s.empty();
}

bool validDate(const string& s)
{
    if (s.size() != 10 || s[2] != '/' || s[5] != '/') return false;
    if (!allDigits(s.substr(0, 2)) || !allDigits(s.substr(3, 2)) ||
        !allDigits(s.substr(6, 4))) return false;
    int day, month, year;
    if (!parseInt(s.substr(0, 2), day) || !parseInt(s.substr(3, 2), month) ||
        !parseInt(s.substr(6, 4), year)) return false;
    if (year < 2000 || year > 2100 || month < 1 || month > 12) return false;
    int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) days[1] = 29;
    return day >= 1 && day <= days[month - 1];
}

int timeMinutes(const string& s)
{
    if (s.size() != 5 || s[2] != ':' || !allDigits(s.substr(0, 2)) ||
        !allDigits(s.substr(3, 2))) return -1;
    int hour, minute;
    if (!parseInt(s.substr(0, 2), hour) || !parseInt(s.substr(3, 2), minute))
        return -1;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;
    return hour * 60 + minute;
}

string timeString(int minutes)
{
    ostringstream result;
    result << setfill('0') << setw(2) << minutes / 60 << ':'
           << setw(2) << minutes % 60;
    return result.str();
}

// Split a line manually into a fixed-size array, without vector or list.
bool splitLine(const string& line, string fields[], int count)
{
    string::size_type start = 0;
    for (int i = 0; i < count; ++i)
    {
        string::size_type pos = line.find('|', start);
        if (i < count - 1 && pos == string::npos) return false;
        if (i == count - 1 && pos != string::npos) return false;
        fields[i] = trim(line.substr(start, pos == string::npos ? pos : pos - start));
        start = pos == string::npos ? line.size() : pos + 1;
    }
    return true;
}

// A missing file means a new installation; an unreadable file disables edits.
bool openDataFile(ifstream& input, const string& file)
{
    input.open(file.c_str());
    if (input.is_open()) return true;
    errno = 0;
    FILE* check = fopen(file.c_str(), "r");
    if (check) { fclose(check); return false; }
    return errno == ENOENT;
}

// Write to a temporary file first, retaining the previous file until replacement.
bool replaceFile(const string& file)
{
    string temp = file + ".tmp", backup = file + ".bak";
    errno = 0;
    bool moved = rename(file.c_str(), backup.c_str()) == 0;
    if (!moved && errno != ENOENT) { remove(temp.c_str()); return false; }
    if (rename(temp.c_str(), file.c_str()) != 0)
    {
        if (moved) rename(backup.c_str(), file.c_str());
        remove(temp.c_str());
        return false;
    }
    if (moved) remove(backup.c_str());
    return true;
}

// ---------- Module 1: User management using a linked list ----------
struct UserNode
{
    string userID, fullName, username, password;
    UserNode* next;
};

class UserManagement
{
private:
    UserNode* head;
    bool readOnly;
    UserManagement(const UserManagement&);
    UserManagement& operator=(const UserManagement&);

    bool duplicate(const string& id, const string& name) const
    {
        for (UserNode* p = head; p != NULL; p = p->next)
            if (p->userID == id || p->username == name) return true;
        return false;
    }

    bool save() const
    {
        ofstream out("users.txt.tmp");
        if (!out) return false;
        for (UserNode* p = head; p != NULL; p = p->next)
            out << p->userID << '|' << p->fullName << '|'
                << p->username << '|' << p->password << '\n';
        out.close();
        if (!out) { remove("users.txt.tmp"); return false; }
        return replaceFile("users.txt");
    }

public:
    UserManagement() : head(NULL), readOnly(false) {}
    ~UserManagement()
    {
        while (head != NULL)
        {
            UserNode* old = head;
            head = head->next;
            delete old;
        }
    }

    void load()
    {
        ifstream in;
        if (!openDataFile(in, "users.txt")) readOnly = true;
        string line, f[4];
        while (getline(in, line))
        {
            if (trim(line).empty()) continue;
            if (!splitLine(line, f, 4) || !validID(f[0]) || !validText(f[1]) ||
                !validID(f[2]) || !validText(f[3]) || duplicate(f[0], f[2]))
            { readOnly = true; continue; }
            UserNode* p = new UserNode;
            p->userID = f[0]; p->fullName = f[1];
            p->username = f[2]; p->password = f[3];
            p->next = head; head = p;
        }
        if (in.bad()) readOnly = true;
        if (readOnly) cout << "Warning: users.txt is invalid or unreadable. "
                          << "Registration is disabled; repair the file first.\n";
    }

    void registerUser()
    {
        if (readOnly) { cout << "Registration is disabled.\n"; return; }
        UserNode record;
        cout << "\n========== REGISTER ==========\n";
        if (!readText("User ID: ", record.userID, true) ||
            !readText("Full Name: ", record.fullName) ||
            !readText("Username: ", record.username, true)) return;
        if (duplicate(record.userID, record.username))
        { cout << "User ID or Username already exists.\n"; return; }
        if (!readText("Password: ", record.password)) return;
        string confirmation;
        if (!readText("Confirm Password: ", confirmation)) return;
        if (confirmation != record.password)
        { cout << "Passwords do not match. Registration cancelled.\n"; return; }
        UserNode* p = new UserNode(record);
        p->next = head; head = p;
        if (!save())
        {
            head = p->next; delete p;
            cout << "Could not save users.txt. Registration cancelled.\n";
            return;
        }
        cout << "Registration successful! You can now log in.\n";
    }

    bool login(string& fullName) const
    {
        string username, password;
        cout << "\n========== LOGIN ==========\n";
        if (!readText("Username: ", username, true) ||
            !readText("Password: ", password)) return false;
        for (UserNode* p = head; p != NULL; p = p->next)
            if (p->username == username && p->password == password)
            {
                fullName = p->fullName;
                cout << "Login successful! Welcome, " << fullName << ".\n";
                return true;
            }
        cout << "Invalid Username or Password. Access denied.\n";
        return false;
    }
};

// ---------- Module 2: Bookings stored in a singly linked list ----------
struct Booking
{
    string bookingID, customerName, court, bookingDate, bookingTime;
    int duration;
    Booking* next;
};

// Each BST node points to a booking owned by the linked list.
struct TreeNode
{
    Booking* record;
    TreeNode* left;
    TreeNode* right;
    TreeNode(Booking* p) : record(p), left(NULL), right(NULL) {}
};

class BookingSystem
{
private:
    Booking* front;
    Booking* rear;
    TreeNode* root;
    bool readOnly;
    BookingSystem(const BookingSystem&);
    BookingSystem& operator=(const BookingSystem&);

    void insertTree(Booking* p)
    {
        TreeNode** link = &root;
        while (*link != NULL)
        {
            if (p->bookingID < (*link)->record->bookingID) link = &(*link)->left;
            else link = &(*link)->right;
        }
        *link = new TreeNode(p);
    }

    // Delete the BST index node before freeing its linked-list record.
    void removeTree(const string& id)
    {
        TreeNode** link = &root;
        while (*link != NULL && (*link)->record->bookingID != id)
            link = id < (*link)->record->bookingID ? &(*link)->left : &(*link)->right;
        if (*link == NULL) return;
        TreeNode* old = *link;
        if (old->left == NULL) *link = old->right;
        else if (old->right == NULL) *link = old->left;
        else
        {
            TreeNode** successor = &old->right;
            while ((*successor)->left != NULL) successor = &(*successor)->left;
            TreeNode* replacement = *successor;
            old->record = replacement->record;
            *successor = replacement->right;
            delete replacement;
            return;
        }
        delete old;
    }

    void append(Booking* p)
    {
        p->next = NULL;
        if (front == NULL) front = rear = p;
        else { rear->next = p; rear = p; }
        insertTree(p);
    }

    bool validBooking(const Booking& b) const
    {
        int start = timeMinutes(b.bookingTime);
        return validID(b.bookingID) && validText(b.customerName) &&
               b.court.size() == 7 && b.court.substr(0, 6) == "Court " &&
               b.court[6] >= 'A' && b.court[6] <= 'E' && validDate(b.bookingDate) &&
               b.duration >= 1 && b.duration <= 4 && start >= OPEN_MINUTES &&
               start + b.duration * 60 <= CLOSE_MINUTES;
    }

    bool conflict(const Booking& b, const Booking* ignore = NULL) const
    {
        int start = timeMinutes(b.bookingTime), end = start + b.duration * 60;
        for (Booking* p = front; p != NULL; p = p->next)
            if (p != ignore && p->court == b.court && p->bookingDate == b.bookingDate)
            {
                int otherStart = timeMinutes(p->bookingTime);
                int otherEnd = otherStart + p->duration * 60;
                if (start < otherEnd && otherStart < end) return true;
            }
        return false;
    }

    bool save() const
    {
        ofstream out("bookings.txt.tmp");
        if (!out) return false;
        for (Booking* p = front; p != NULL; p = p->next)
            out << p->bookingID << '|' << p->customerName << '|' << p->court << '|'
                << p->bookingDate << '|' << p->bookingTime << '|' << p->duration << '\n';
        out.close();
        if (!out) { remove("bookings.txt.tmp"); return false; }
        return replaceFile("bookings.txt");
    }

    bool editAllowed() const
    {
        if (!readOnly) return true;
        cout << "Changes are disabled. Repair bookings.txt first.\n";
        return false;
    }

    bool readDetails(Booking& b)
    {
        if (!readText("Customer Name: ", b.customerName)) return false;
        int courtChoice;
        cout << "1. Court A\n2. Court B\n3. Court C\n4. Court D\n5. Court E\n";
        if (!readInt("Select Court: ", courtChoice, 1, 5)) return false;
        b.court = "Court "; b.court += char('A' + courtChoice - 1);
        do
        {
            if (!readText("Booking Date (DD/MM/YYYY): ", b.bookingDate)) return false;
            if (validDate(b.bookingDate)) break;
            cout << "Invalid calendar date. Use DD/MM/YYYY, year 2000-2100.\n";
        } while (true);
        do
        {
            if (!readText("Start Time (HH:MM, 24-hour): ", b.bookingTime)) return false;
            int start = timeMinutes(b.bookingTime);
            if (start >= OPEN_MINUTES && start <= CLOSE_MINUTES - 60) break;
            cout << "Start time must be between 08:00 and 21:00.\n";
        } while (true);
        do
        {
            if (!readInt("Duration in hours (1-4): ", b.duration, 1, 4)) return false;
            if (validBooking(b)) break;
            cout << "Booking must finish by 22:00. Choose a shorter duration.\n";
        } while (true);
        return true;
    }

    void displayOne(const Booking* p) const
    {
        cout << "Booking ID    : " << p->bookingID << '\n'
             << "Customer Name : " << p->customerName << '\n'
             << "Court         : " << p->court << '\n'
             << "Booking Date  : " << p->bookingDate << '\n'
             << "Start Time    : " << p->bookingTime << '\n'
             << "End Time      : " << timeString(timeMinutes(p->bookingTime) +
                                                  p->duration * 60) << '\n'
             << "Duration      : " << p->duration << " hour(s)\n"
             << fixed << setprecision(2)
             << "Booking Fee   : RM " << p->duration * HOURLY_RATE << '\n';
    }

public:
    BookingSystem() : front(NULL), rear(NULL), root(NULL), readOnly(false) {}
    ~BookingSystem()
    {
        while (front != NULL)
        {
            Booking* old = front;
            removeTree(old->bookingID);
            front = front->next;
            delete old;
        }
        rear = NULL;
    }

    Booking* sequentialSearch(const string& id, int& comparisons) const
    {
        comparisons = 0;
        for (Booking* p = front; p != NULL; p = p->next)
        {
            ++comparisons;
            if (p->bookingID == id) return p;
        }
        return NULL;
    }

    Booking* bstSearch(const string& id, int& comparisons) const
    {
        comparisons = 0;
        TreeNode* p = root;
        while (p != NULL)
        {
            ++comparisons;
            if (p->record->bookingID == id) return p->record;
            p = id < p->record->bookingID ? p->left : p->right;
        }
        return NULL;
    }

    void load()
    {
        ifstream in;
        if (!openDataFile(in, "bookings.txt")) readOnly = true;
        string line, f[6];
        while (getline(in, line))
        {
            if (trim(line).empty()) continue;
            Booking b; int comparisons;
            if (!splitLine(line, f, 6) || !parseInt(f[5], b.duration))
            { readOnly = true; continue; }
            b.bookingID = f[0]; b.customerName = f[1]; b.court = f[2];
            b.bookingDate = f[3]; b.bookingTime = f[4]; b.next = NULL;
            if (!validBooking(b) || bstSearch(b.bookingID, comparisons) != NULL ||
                conflict(b)) { readOnly = true; continue; }
            append(new Booking(b));
        }
        if (in.bad()) readOnly = true;
        if (readOnly) cout << "Warning: bookings.txt is invalid or unreadable. "
                          << "Changes are disabled; repair the file first.\n";
    }

    void displayBookings() const
    {
        cout << "\n========== BOOKING RECORDS ==========\n";
        if (front == NULL) { cout << "No booking records found.\n"; return; }
        int number = 1;
        for (Booking* p = front; p != NULL; p = p->next)
        { cout << "\nBooking #" << number++ << '\n'; displayOne(p); }
    }

    void addBooking()
    {
        if (!editAllowed()) return;
        Booking b; b.next = NULL;
        cout << "\n========== ADD NEW BOOKING ==========\n";
        if (!readText("Booking ID: ", b.bookingID, true)) return;
        int comparisons;
        if (bstSearch(b.bookingID, comparisons) != NULL)
        { cout << "Booking ID already exists.\n"; return; }
        if (!readDetails(b)) return;
        if (conflict(b)) { cout << "Court is already booked during this time.\n"; return; }
        Booking* oldRear = rear;
        Booking* p = new Booking(b);
        append(p);
        if (!save())
        {
            removeTree(p->bookingID);
            if (oldRear == NULL) front = rear = NULL;
            else { oldRear->next = NULL; rear = oldRear; }
            delete p;
            cout << "Could not save bookings.txt. Addition cancelled.\n";
            return;
        }
        cout << "Booking added successfully!\n";
        displayOne(p);
    }

    void updateBooking()
    {
        if (!editAllowed()) return;
        string id; int comparisons;
        cout << "\n========== UPDATE BOOKING ==========\n";
        if (!readText("Booking ID to update: ", id, true)) return;
        Booking* p = bstSearch(id, comparisons);
        if (p == NULL) { cout << "Booking not found.\n"; return; }
        displayOne(p);
        cout << "Enter replacement details. Booking ID stays the same.\n";
        Booking replacement = *p;
        if (!readDetails(replacement)) return;
        if (conflict(replacement, p))
        { cout << "Court is already booked during this time. Update cancelled.\n"; return; }
        Booking original = *p;
        *p = replacement;
        if (!save())
        { *p = original; cout << "Could not save bookings.txt. Update cancelled.\n"; return; }
        cout << "Booking updated successfully!\n";
        displayOne(p);
    }

    void deleteBooking()
    {
        if (!editAllowed()) return;
        string id;
        cout << "\n========== DELETE BOOKING ==========\n";
        if (!readText("Booking ID to delete: ", id, true)) return;
        Booking* previous = NULL; Booking* p = front;
        while (p != NULL && p->bookingID != id) { previous = p; p = p->next; }
        if (p == NULL) { cout << "Booking not found.\n"; return; }
        displayOne(p);
        string answer;
        if (!readText("Confirm deletion (Y/N): ", answer)) return;
        if (answer != "Y" && answer != "y") { cout << "Deletion cancelled.\n"; return; }
        Booking* oldRear = rear;
        if (previous == NULL) front = p->next; else previous->next = p->next;
        if (rear == p) rear = previous;
        if (!save())
        {
            if (previous == NULL) front = p; else previous->next = p;
            rear = oldRear;
            cout << "Could not save bookings.txt. Deletion cancelled.\n";
            return;
        }
        removeTree(id);
        delete p;
        cout << "Booking deleted successfully!\n";
    }

    void searchBooking(bool useBST) const
    {
        string id; int comparisons;
        cout << (useBST ? "\n========== BST SEARCH ==========\n" :
                          "\n========== SEQUENTIAL SEARCH ==========\n");
        if (!readText("Booking ID to search: ", id, true)) return;
        Booking* p = useBST ? bstSearch(id, comparisons) : sequentialSearch(id, comparisons);
        if (p == NULL) cout << "Booking not found.\n";
        else { cout << "Booking found!\n"; displayOne(p); }
        cout << "Records/nodes visited: " << comparisons << '\n';
    }

    void showSummary() const
    {
        int count = 0, hours = 0, courtCounts[5] = {0, 0, 0, 0, 0};
        for (Booking* p = front; p != NULL; p = p->next)
        { ++count; hours += p->duration; ++courtCounts[p->court[6] - 'A']; }
        cout << "\n========== BOOKING SUMMARY ==========\n"
             << "Total Bookings     : " << count << '\n'
             << "Total Booked Hours : " << hours << '\n'
             << fixed << setprecision(2)
             << "Hourly Rate        : RM " << HOURLY_RATE << '\n'
             << "Total Booking Fees : RM " << hours * HOURLY_RATE << '\n';
        for (int i = 0; i < 5; ++i)
            cout << "Court " << char('A' + i) << " Bookings   : " << courtCounts[i] << '\n';
    }

    // Return true for program exit, false for logout.
    bool mainMenu(const string& fullName)
    {
        while (cin)
        {
            cout << "\n========================================\n"
                 << "   BADMINTON COURT BOOKING SYSTEM\n"
                 << "========================================\n"
                 << "Logged in as: " << fullName << '\n'
                 << "1. View All Booking Records\n2. Add New Booking Record\n"
                 << "3. Update Booking Record\n4. Delete Booking Record\n"
                 << "5. Search by Linked List\n6. Search by BST\n"
                 << "7. Booking Summary\n8. Logout\n9. Exit Program\n";
            int choice;
            if (!readInt("Enter your choice: ", choice, 1, 9)) return true;
            if (choice == 1) displayBookings();
            else if (choice == 2) addBooking();
            else if (choice == 3) updateBooking();
            else if (choice == 4) deleteBooking();
            else if (choice == 5) searchBooking(false);
            else if (choice == 6) searchBooking(true);
            else if (choice == 7) showSummary();
            else if (choice == 8) { cout << "Logged out successfully.\n"; return false; }
            else return true;
        }
        return true;
    }
};

int main()
{
    UserManagement users;
    BookingSystem bookings;
    cout << "Welcome to Badminton Court Booking System!\n";
    users.load(); bookings.load();
    while (cin)
    {
        cout << "\n========== LOGIN MENU ==========\n"
             << "1. Register\n2. Login\n3. Exit\n";
        int choice;
        if (!readInt("Enter your choice: ", choice, 1, 3)) break;
        if (choice == 1) users.registerUser();
        else if (choice == 2)
        {
            string fullName;
            if (users.login(fullName) && bookings.mainMenu(fullName)) break;
        }
        else break;
    }
    cout << "\nThank you for using the system.\n";
    return 0;
}
