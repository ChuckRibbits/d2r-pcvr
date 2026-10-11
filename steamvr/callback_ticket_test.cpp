#include "callback_ticket.h"
#include <cassert>
int main() {
    framevr::CallbackTicket t;
    auto old=t.Start(100);assert(old&&t.Start(101)==0);
    assert(!t.Expired(599,500)&&t.Expired(600,500));
    t.Cancel();auto fresh=t.Start(600);assert(fresh&&fresh!=old);
    assert(!t.Claim(old)); // discarded transition callback delivered late
    assert(t.Age(610)==10&&t.Start(610)==0);
    assert(t.Claim(fresh)&&!t.Claim(fresh)&&t.Age(611)==0);
    auto failed=t.Start(700);assert(t.Claim(failed)); // scheduling failure
    auto next=t.Start(701);assert(next!=failed&&!t.Claim(failed));
    t.Cancel();assert(!t.Claim(next));
}
