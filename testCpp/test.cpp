#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h> // for sockaddr_in
#include <sys/types.h>  // for socket
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <list>

std::list<int> li;

int main()
{
    li.push_front(10);
    std::list<int>::iterator it = std::find(li.begin(), li.end(), 10);
    std::list<int>::iterator is = std::find(li.begin(), li.end(), 10);
    *it = 20;
    *is = 30;
    printf("%d",*li.begin());
}