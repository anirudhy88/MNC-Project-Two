#include <iostream>
#include <sstream>
#include <limits.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include <list>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>

using namespace std;
#define STDIN 0
#define UPDATEFIELDSNUM 16
#define SERVINFO 48
#define NEIGHBOURINFO 96
#define MAXDATASIZE 544
#define INT_MAX_NEW 2147400000
#define SHRT_MAX_NEW 32000 

// Network Elements Information
struct NetworkElementsInfo {
    short int id;
    string ipAddress;
    short int portNumber;
    bool isNeighbour ;
};
// Current Elements' Information
struct ThisElementInfo {
    short int id;
    string ipAddress;
    short int portNumber;
    friend ostream& operator << (ostream& os, const NetworkElementsInfo& m);
};

// Current elements' neighbours
struct ThisElementNeighbourInfo {
    short int id;
    short int cost;
    char ipAddress[INET_ADDRSTRLEN];
    short int portNumber;
    friend ostream& operator << (ostream& os, const ThisElementNeighbourInfo& m);
};
// DATA STRUCTURE OF THE ROUTNG TABLE
struct RoutingTableElement {
    short int destinationServerId;
    short int nextHopServerId;
    short int costOfPath;
};

// DATA STRUCTURE OF THE UPDATE MESSAGE
struct NeighbourDistanceVectorDetails {
    short int numOfUpdateFields; // 2 bytes
    char ipAddress[4];           // 4 bytes
    short int buffer[5];         // 10 bytes - 2 per each server in routing table
    short int portNumber;        // 2 bytes
    short int neighbourIdArr[5]; // 10 bytes - 2 per each server in routing table
    short int costArr[5];        // 10 bytes - 2 per each server in routing table
    short int portnumberArr[5];  // 10 bytes - 2 per each server in routing table
    char ipAddressArr[5][4];     // 20 bytes - 4 per each server in routing table
};

fd_set master;
short int serverCount, neighbourCount;
list<NetworkElementsInfo> networkElementsList;
list<ThisElementNeighbourInfo> thisElementNeighboursList;
list<NeighbourDistanceVectorDetails> listOfNeighbourDistanceVectors;
ThisElementInfo thisElementInfo;
list<RoutingTableElement> routingTableList;
short int costArray[6][6];
int packet;
int socket_fd;
NeighbourDistanceVectorDetails x3;
bool cantProceed;
string myIp; // Stores ip address determined by getMyIP function
bool flagTimer[5];
int timerCount[5];

ostream& operator << (ostream& os, const NetworkElementsInfo& m)
{
    os << m.id << " "  << m.ipAddress << " " << m.portNumber << endl;
    return os;
}
ostream& operator << (ostream& os, const ThisElementNeighbourInfo& m)
{
    os << m.id << " "  << m.cost << endl;
    return os;
}
uint64_t getTime()
{
    struct timeval timeVal;
    gettimeofday(&timeVal, NULL);
    return timeVal.tv_sec;
}
void displayCostArray()
{
    cout << "--------------------" << endl;
    cout << "Cost Array is " << endl;
    cout << "--------------------" << endl;
    cout << "  1   2   3" << endl;
    for(int i=1; i <=3; i++)
    {
        cout << i << " " ;
        for(int j=1; j<=3; j++)
        {
            if(costArray[i][j] != SHRT_MAX_NEW)
                cout << costArray[i][j] << "   " ;
            else
                cout << "inf" << "   " ;
        }
        cout << endl;
    }
}
void getMyIp()
{
    const char* googleIP =  "8.8.8.8";
    uint16_t googlePort = 53;
    struct sockaddr_in servInfo;
    sockaddr_in name;
    socklen_t nameLength = sizeof(name);
    memset(&servInfo, 0, sizeof(servInfo));
    servInfo.sin_family = AF_INET;
    servInfo.sin_addr.s_addr = inet_addr(googleIP);
    servInfo.sin_port = htons(googlePort);
    char ip[INET_ADDRSTRLEN];
    // create a UDP socket
    int socket_fd_g = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_fd_g != -1)
    {
        // Send packets out
        int connec = connect(socket_fd_g, (const sockaddr*) &servInfo, sizeof(servInfo));
        if( connec != -1 )
        {
            // getsockname use to get local address
            getsockname(socket_fd_g, (sockaddr*) &name, &nameLength);
            inet_ntop(AF_INET, &name.sin_addr, ip, INET_ADDRSTRLEN);
            myIp = ip;
            close(socket_fd_g);
        }
    }
    cout << "My IP is : " << myIp << endl;
}
void display()
{
    cout << "display SUCCESS" << endl;
    cout << "--------------------" << endl;
    cout << "Routing Table is " << endl;
    cout << "--------------------" << endl;
    // Display the current routing table
    int arrToDis[5];
    for(int i=1; i<=5 ; i++)
    {
        arrToDis[i-1] = i;
    }
    cout << "Destination ID  " << "Next Hop ID  " << "Cost" <<endl;
    for(int j=0;j<=4 ; j++)
    {
        std::list <RoutingTableElement>::iterator iterator3;
        for(iterator3 = routingTableList.begin(); iterator3 != routingTableList.end(); ++iterator3)
        {
            if(arrToDis[j] == iterator3->destinationServerId)
            {
                cout << iterator3->destinationServerId;
                cout << "    " << iterator3->nextHopServerId;
                if(iterator3->costOfPath != SHRT_MAX_NEW)
                    cout << "    " << iterator3->costOfPath <<endl;
                else
                    cout << "   inf" << endl;
            }
        }
    }
}
void buildRoutingTable() 
{
    RoutingTableElement routingTableElement;
    std::list <NetworkElementsInfo>::iterator iterator2;
    for(iterator2 = networkElementsList.begin(); iterator2 != networkElementsList.end(); ++iterator2)
    {
        routingTableElement.destinationServerId = iterator2->id;
        if(iterator2->isNeighbour) 
        {
            routingTableElement.nextHopServerId = routingTableElement.destinationServerId;
            std::list <ThisElementNeighbourInfo>::iterator iterator1;
            for(iterator1 = thisElementNeighboursList.begin(); iterator1 != thisElementNeighboursList.end(); ++iterator1)
            {
                if(iterator1->id == iterator2->id)
                {
                    routingTableElement.costOfPath = iterator1->cost;
                }
            }
        }
        else
        {
            if(iterator2->id != thisElementInfo.id)
            {
                routingTableElement.costOfPath = SHRT_MAX_NEW;
                routingTableElement.nextHopServerId = SHRT_MAX_NEW;
            }
            else
            {
                routingTableElement.costOfPath = 0;
                routingTableElement.nextHopServerId = thisElementInfo.id;
            }
        }
        routingTableList.push_back(routingTableElement);
    }
}
void calculateDistanceVector()
{
    // Need to update the data structure used to maintain this cost
    list<NeighbourDistanceVectorDetails> listOfNeighbourDistanceVectors ;
    list<NeighbourDistanceVectorDetails>::iterator iterator4;
    list <ThisElementNeighbourInfo>::iterator iterator5;
    std::list <NetworkElementsInfo>::iterator iterator20;
    bool alreadyPushed = false;
    // check for the already existing record in the list and modify the exisiting record.
    if(listOfNeighbourDistanceVectors.size() == 0)
    {
        listOfNeighbourDistanceVectors.push_front(x3);
        alreadyPushed = true;
    }
    else
    {
        for(iterator4 = listOfNeighbourDistanceVectors.begin(); iterator4 != listOfNeighbourDistanceVectors.end(); ++iterator4)
        {
            if( ((int)(iterator4->ipAddress[0]&0xFF) == ((int)x3.ipAddress[0]&0xFF)) && ((int)(iterator4->ipAddress[1]&0xFF) == ((int)x3.ipAddress[1]&0xFF)) && ((int)(iterator4->ipAddress[2]&0xFF) == ((int)x3.ipAddress[2]&0xFF)) && ((int)(iterator4->ipAddress[3]&0xFF) == ((int)x3.ipAddress[3]&0xFF)) ) 
            {
                iterator4->portNumber = x3.portNumber;
                for(int j=0;j<=4; j++)
                {
                    iterator4->neighbourIdArr[j] = x3.neighbourIdArr[j];
                    iterator4->costArr[j] = x3.costArr[j];
                    iterator4->portnumberArr[j] = x3.portnumberArr[j];
                }
            }
            else
            {
                if(!alreadyPushed)
                    listOfNeighbourDistanceVectors.push_front(x3);
            }
        }
    }
    int id = 0;
    for(iterator20 = networkElementsList.begin(); iterator20 != networkElementsList.end(); ++iterator20)
    {
        stringstream ss1,ss2,ss3,ss4;
        string s1,s2,s3,s4;
        ss1<<(int)(x3.ipAddress[0]&0xFF);
        ss1>> s1;
        ss2<<(int)(x3.ipAddress[1]&0xFF);
        ss2>>s2; 
        ss3<<(int)(x3.ipAddress[2]&0xFF);
        ss3>>s3;
        ss4<<(int)(x3.ipAddress[3]&0xFF);
        ss4>>s4;
        string newIp = s1 +"."+ s2 +"."+ s3+"."+ s4;
        if(newIp.compare(iterator20->ipAddress) == 0)
        {
            id = iterator20->id;
        }
    }
    for(int z=0; z<=4; z++)
    {
        costArray[id][z+1] = x3.costArr[z];
    }
    cout << "RECEIVED A MESSAGE FROM SERVER : "<<id <<endl;
    for(int i=1;i<=5;i++)
    {
        costArray[thisElementInfo.id][i] = SHRT_MAX_NEW;
    }

    for(int i=1;i<=5;i++)
    {
        for(int j=1;j<=5;j++)
        {
            if(i == j)
                costArray[i][j] = 0;
        }
    }
    std::list <NetworkElementsInfo>::iterator iterator2;
    std::list <ThisElementNeighbourInfo>::iterator iterator1;
    std::list <RoutingTableElement>::iterator iterator3;

    // For every element in the network we have to see as the routing table consists of all dstinations
    for(iterator2 = networkElementsList.begin(); iterator2 != networkElementsList.end(); ++iterator2)
    {
        if(iterator2->id == thisElementInfo.id)
        {
            continue;
        }
        else
        {
            for(iterator1 = thisElementNeighboursList.begin(); iterator1 != thisElementNeighboursList.end(); ++iterator1)
            {
                if(costArray[thisElementInfo.id][iterator2->id] >( iterator1->cost+ costArray[iterator1->id][iterator2->id]))
                {
                    costArray[thisElementInfo.id][iterator2->id] = iterator1->cost+ costArray[iterator1->id][iterator2->id];
                    for(iterator3 = routingTableList.begin(); iterator3 != routingTableList.end(); ++iterator3)
                    {
                        if(iterator3->destinationServerId == iterator2->id)
                        {
                            iterator3->nextHopServerId = iterator1->id;
                            iterator3->costOfPath=costArray[thisElementInfo.id][iterator2->id]; 
                        }
                    }
                }
            } 
        }
    }
}
void updateRoutingTable()
{
    for(int i=1;i<=5;i++)
    {
        costArray[thisElementInfo.id][i] = SHRT_MAX_NEW;
    }
    costArray[thisElementInfo.id][thisElementInfo.id] = 0;
    std::list <NetworkElementsInfo>::iterator iterator2;
    std::list <ThisElementNeighbourInfo>::iterator iterator1;
    std::list <RoutingTableElement>::iterator iterator3;
    bool flagSpecial;

    // For every element in the network we have to see as the routing table consists of all dstinations
    for(iterator2 = networkElementsList.begin(); iterator2 != networkElementsList.end(); ++iterator2)
    {
        if(iterator2->id == thisElementInfo.id)
        {
            continue;
        }
        else
        {

            for(iterator1 = thisElementNeighboursList.begin(); iterator1 != thisElementNeighboursList.end(); ++iterator1)
            {
                if(costArray[thisElementInfo.id][iterator2->id] >( iterator1->cost+ costArray[iterator1->id][iterator2->id]))
                {
                    costArray[thisElementInfo.id][iterator2->id] = iterator1->cost+ costArray[iterator1->id][iterator2->id];
                    for(iterator3 = routingTableList.begin(); iterator3 != routingTableList.end(); ++iterator3)
                    {
                        if(iterator3->destinationServerId == iterator2->id)
                        {
                            iterator3->nextHopServerId = iterator1->id;
                            iterator3->costOfPath=costArray[thisElementInfo.id][iterator2->id]; 
                        }
                    }
                }
            }
        }
    }
}
void readTopologyFile(string topologyFile)
{
    fstream myFile;
    string line;
    string delimiter = " ";
    string token[4] = {};
    size_t position = 0;
    int numOfElements= 0;
    int numOfNeighbours= 0;
    int track = 0;
    string dot = ".";
    size_t isNetworkInfo = 0;
    ThisElementNeighbourInfo neighbourInfo;
    NetworkElementsInfo networkInfo;

    myFile.open(topologyFile.c_str());
    if(!myFile)
    {
        cerr << "Can't open the topology file" << endl;
    }
    while (getline(myFile, line))
    {
        int count = 1;
        while((position = line.find(delimiter)) != string::npos)
        {
            token[count] = line.substr(0,position);
            line.erase(0, position+delimiter.length());
            token[count+1] = line.substr(0,100);
            count++;
        }
        if(count == 1)
        {
            if(track == 0)
            {
                track++;
                numOfElements  = atoi(line.c_str());
                serverCount = numOfElements;
            }
            else
            {
                numOfNeighbours = atoi(line.c_str());
                neighbourCount = numOfNeighbours;
            }
        }
        else
        {
            isNetworkInfo = token[2].find(dot);
            if(isNetworkInfo != string::npos)
            {
                // The line contains IP address
                networkInfo.id = atoi(token[1].c_str());
                networkInfo.ipAddress = token[2];
                networkInfo.portNumber = atoi(token[3].c_str());
                networkElementsList.push_back(networkInfo);
            }
            else
            {
                // The line indicates this elements neighbours
                neighbourInfo.id = atoi(token[2].c_str());
                neighbourInfo.cost = atoi(token[3].c_str()); 
                thisElementNeighboursList.push_back(neighbourInfo);
            }
        }
    }
    std::list <ThisElementNeighbourInfo>::iterator iterator1;
    std::list <NetworkElementsInfo>::iterator iterator2;
    for(iterator1 = thisElementNeighboursList.begin(); iterator1 != thisElementNeighboursList.end(); ++iterator1)
    {
        for(iterator2 = networkElementsList.begin(); iterator2 != networkElementsList.end(); ++iterator2)
        {
            if(iterator1->id == iterator2->id)
            {
                string ip = iterator2->ipAddress;
                char z[INET_ADDRSTRLEN];
                strcpy(z, ip.c_str()); 
                strcpy(iterator1->ipAddress, z); 
                iterator1->portNumber= iterator2->portNumber;
            }
        }
    } 


    for(iterator2 = networkElementsList.begin(); iterator2 != networkElementsList.end(); ++iterator2)
    {
        for(iterator1 = thisElementNeighboursList.begin(); iterator1 != thisElementNeighboursList.end(); ++iterator1)
        {
            if(iterator1->id == iterator2->id)
            {
                iterator2->isNeighbour = true;
            }

        }

    }
}
void  sendDistanceVectorUpdate(NeighbourDistanceVectorDetails *mess)
{
    int s = 0;    
    int *size=&s;
    *size  = UPDATEFIELDSNUM + SERVINFO + neighbourCount*5;
    int *sizeTotal = size;
    char * bufferToSend = (char*)malloc(sizeof(char)*(*sizeTotal));
    mess->numOfUpdateFields = serverCount;
    mess->portNumber = thisElementInfo.portNumber;
    string delimiter = ".";
    int intIPMain[4];
    size_t pos = 0;
    string tokenMain[5] = {};
    int pobeyMain = 1;
    string inputMain =  thisElementInfo.ipAddress;
    while((pos = inputMain.find(delimiter)) != string::npos)
    {
        tokenMain[pobeyMain] = inputMain.substr(0,pos);
        inputMain.erase(0,pos+delimiter.length());
        tokenMain[pobeyMain+1] =inputMain.substr(0,100);
        pobeyMain++;
    }
    for(int j = 1; j<=4; j++)
    {
        std::istringstream iss(tokenMain[j]);
        iss>>intIPMain[j-1];
    }
    for(int i=0; i<=3; i++)
        for(int i = 0 ; i <= 3; i++)
        {
            mess->ipAddress[i] = 0xFF & intIPMain[i];
        }

    int edo =0 ;
    std::list <NetworkElementsInfo>::iterator iterator2;
    std::list <RoutingTableElement>::iterator iterator3;
    for(iterator3 = routingTableList.begin(); iterator3 != routingTableList.end(); ++iterator3)

    {
        for(iterator2 = networkElementsList.begin(); iterator2 != networkElementsList.end(); ++iterator2)
        {
            if(iterator3->destinationServerId == iterator2->id)
            {
                mess->neighbourIdArr[iterator2->id-1] = iterator2->id;
                mess->buffer[iterator2->id-1] =  0;
                mess->costArr[iterator2->id-1] = iterator3->costOfPath;
                mess->portnumberArr[iterator2->id-1] = iterator2->portNumber;

                string delimiter = ".";
                size_t pos = 0;
                string token[5] = {};
                int pobey = 1;
                string input =  iterator2->ipAddress;
                int intIP[4];
                while((pos = input.find(delimiter)) != string::npos)
                {
                    token[pobey] = input.substr(0,pos);
                    input.erase(0,pos+delimiter.length());
                    token[pobey+1] =input.substr(0,100);
                    pobey++;
                }
                for(int j = 1; j<=4; j++)
                {
                    std::istringstream iss(token[j]);
                    iss>>intIP[j-1];
                }
                for(int i = 0 ; i <= 3; i++)
                {
                    mess->ipAddressArr[iterator2->id-1][i] = 0xFF & intIP[i];
                }
                edo++;
            }
        }
    }
}
void update(string s1, string s2, string s3) 
{
    //creating UDP packet
    std::list <NetworkElementsInfo>::iterator iterat;
    int id1 = atoi(s1.c_str());
    int id2 = atoi(s2.c_str());
    short int id3 = atoi(s3.c_str());
    if(s3.compare("inf") == 0)
    {
        std::list <ThisElementNeighbourInfo>::iterator iterator1 = thisElementNeighboursList.begin();
        while(iterator1 != thisElementNeighboursList.end())
        {
            if(iterator1->id == id2)
            {
                thisElementNeighboursList.erase(iterator1++);
                neighbourCount--;
            }
            else
            {
                ++iterator1;
            }
        }
        costArray[id1][id2] = SHRT_MAX_NEW;  
        for(iterat = networkElementsList.begin(); iterat != networkElementsList.end(); ++iterat)
        {
            if(iterat->id == id2)
            {
                iterat->isNeighbour = false;
            }
        }
    }
    else
    {
        costArray[id1][id2] = id3;
        std::list <ThisElementNeighbourInfo>::iterator iteratorOk;
        for(iteratorOk = thisElementNeighboursList.begin(); iteratorOk != thisElementNeighboursList.end(); ++iteratorOk)
        {
            if(iteratorOk->id == id2)
            {
                iteratorOk->cost = id3;
            }
        }
    }
    updateRoutingTable();
    string senderIp;
    int senderPort= 0;
    for(iterat = networkElementsList.begin(); iterat != networkElementsList.end(); ++iterat)
    {
        if(iterat->id == id2)
        {
            senderIp = iterat->ipAddress;
            senderPort = iterat->portNumber;
        }
    }
    string buffer;
    char buf[1026];
    char z[INET_ADDRSTRLEN];
    buffer = "UPDATE " + s2 + " " + s1 + " "+ s3;
    strcpy(buf, buffer.c_str());
    strcpy(z, senderIp.c_str());
    struct sockaddr_in somethingda;
    struct in_addr edo;
    somethingda.sin_family = AF_INET;
    inet_aton(z, &edo);
    somethingda.sin_addr = edo;
    int messSize = sizeof(NeighbourDistanceVectorDetails);
    somethingda.sin_port = htons(senderPort);
    NeighbourDistanceVectorDetails  *mess = (NeighbourDistanceVectorDetails *)malloc(sizeof(NeighbourDistanceVectorDetails));

    mess->numOfUpdateFields = 99;
    mess->buffer[0]= id1; // sending destin id for which the server has to change cost
    mess->buffer[1]= id3;// sending cost
    mess->numOfUpdateFields = 99;
    size_t send = sendto(socket_fd,(void *) mess, messSize, 0, (struct sockaddr*)&(somethingda), sizeof somethingda);
    if(send < 0)
    {
        cout << "update Send Failure" << endl;
    }
    else
    {
        cout << "update SUCCESS" << endl;
    }
}
void crash()
{
    cout << "crash SUCCESS"<<endl;
    close(socket_fd);
    exit(0);
}
void packets() 
{
    cout << "packets SUCCESS" << endl;
    int packetCountCurrent = packet;
    packet = 0;
    cout << "Number of packets reveived: " <<packetCountCurrent<<endl;
}
void disable(string id)
{
    cout << "disable SUCCESS" << endl;
    int id1 = 0;
    id1 = atoi(id.c_str());

    std::list <NetworkElementsInfo>::iterator iterat;
    std::list <ThisElementNeighbourInfo>::iterator iterator1 = thisElementNeighboursList.begin();
    while(iterator1 != thisElementNeighboursList.end())
    {
        if(iterator1->id == id1)
        {
            thisElementNeighboursList.erase(iterator1++);
            neighbourCount--;
        }
        else
        {
            ++iterator1;
        }
    }
    costArray[thisElementInfo.id][id1] = SHRT_MAX_NEW;
    for(iterat = networkElementsList.begin(); iterat != networkElementsList.end(); ++iterat)
    {
        if(iterat->id == id1)
        {
            iterat->isNeighbour = false;
        }
    }
    updateRoutingTable(); 
}
void step()
{
    // sending routing update
    NeighbourDistanceVectorDetails  *mess = (NeighbourDistanceVectorDetails *)malloc(sizeof(NeighbourDistanceVectorDetails));
    sendDistanceVectorUpdate(mess);

    int send = 0;
    std::list <ThisElementNeighbourInfo>::iterator iterator1;
    for(iterator1 = thisElementNeighboursList.begin(); iterator1 != thisElementNeighboursList.end(); ++iterator1)
    {
        char z[INET_ADDRSTRLEN];
        strcpy(z, iterator1->ipAddress);
        int errno;
        struct sockaddr_in somethingda;
        struct in_addr edo;
        somethingda.sin_family = AF_INET;
        inet_aton(z, &edo);
        somethingda.sin_addr = edo;
        somethingda.sin_port = htons(iterator1->portNumber);
        int messSize = sizeof(NeighbourDistanceVectorDetails);
        send = sendto(socket_fd,(void *) mess, messSize, 0, (struct sockaddr*)&(somethingda), sizeof somethingda);
    }
    if(send < 0) 
    {
        cout << "step Send failure" << endl;
    }
    else
    {
        cout << "step SUCCESS" << endl;
    }

}
int main(int argc, char* argv[]) 
{
    string arg1 = argv[1];
    string arg2 = argv[2];
    string arg3 = argv[3];
    string arg4 = argv[4];

    for(int i=0; i<=4; i++)
        flagTimer[i] = false;
    if(argc == 5) 
    {
        if (arg1 == "-t") 
        {
            if(arg2.find(".txt") != string::npos)
            {
            }
            else
            {
                cantProceed = true;
            }
        }
        if(arg3 == "-i")
        {
        }
    }
    else
    {
        cout << "Fewer arguments given by the user" << endl;
    }

    if(!cantProceed)
    {
        readTopologyFile(arg2);
        getMyIp();

        for(list<NetworkElementsInfo>::const_iterator iterator = networkElementsList.begin(), end = networkElementsList.end(); iterator != end; ++iterator)
        {
            if(iterator->ipAddress == myIp)
            {
                thisElementInfo.id = iterator->id;
                thisElementInfo.ipAddress = iterator->ipAddress;
                thisElementInfo.portNumber= iterator->portNumber;
            }
        }

        //creating UDP packet
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(socket_fd == -1)
            cout << "Error in creating a UDP soket" << endl;
        struct sockaddr_in myDetails, sourceAddress;
        memset(&myDetails, 0, sizeof(myDetails));

        myDetails.sin_family = AF_INET;
        myDetails.sin_addr.s_addr = htonl(INADDR_ANY);
        myDetails.sin_port = htons(thisElementInfo.portNumber);

        if(bind(socket_fd, (struct sockaddr *)&myDetails, sizeof(myDetails)) < 0 )
            cout << "Unable to bnd the socket" << endl;


        NeighbourDistanceVectorDetails  *mess = (NeighbourDistanceVectorDetails *)malloc(sizeof(NeighbourDistanceVectorDetails));

        // Building the cost array
        // Initialising with infinty values
        for(int i=1; i<=5; i++)
        {
            for(int j=1; j<=5; j++)
            {
                costArray[i][j] = SHRT_MAX_NEW;
            }
        }

        costArray[thisElementInfo.id][thisElementInfo.id] = 0;
        std::list <ThisElementNeighbourInfo>::iterator iterator3;
        for(iterator3 = thisElementNeighboursList.begin(); iterator3 != thisElementNeighboursList.end(); ++iterator3)
        {
            costArray[thisElementInfo.id][iterator3->id] = iterator3->cost;
        }
        buildRoutingTable();
        sendDistanceVectorUpdate(mess);
        int messSize = sizeof(NeighbourDistanceVectorDetails);
        fd_set read_fds;
        int fdmax;
        FD_ZERO(&master);
        FD_ZERO(&read_fds);
        FD_SET(STDIN, &master);
        FD_SET(socket_fd, &master);
        fdmax = socket_fd;
        char buf[1024];
        int i, ite;
        bool discard = false;
        int optionGiven = 0;
        string options[] = {"update", "step", "packets", "disable", "crash", "display"};
        struct timeval timeOut;
        timeOut.tv_sec = atoi(arg4.c_str());
        timeOut.tv_usec = 0 ;
        char ipAddress[INET_ADDRSTRLEN];
        int recvBytes;
        time_t currTimer;
        uint64_t timerOne = 0; 
        uint64_t timerTwo =0 ; 
        uint64_t timerThree = 0;
        for(;;)
        {
            read_fds = master;
            int sele;

            timerOne = getTime();
            sele = select(fdmax+1, &read_fds, NULL, NULL, &timeOut);
            if(sele == -1)
            {
                cout << "Error : Doing select" << endl;
            }
            if(sele == 0)
            {
                bool specialFlag;
                sendDistanceVectorUpdate(mess);
                std::list <ThisElementNeighbourInfo>::iterator iterator1;
                for(iterator1 = thisElementNeighboursList.begin(); iterator1 != thisElementNeighboursList.end(); ++iterator1)
                {
                    char z[INET_ADDRSTRLEN];
                    strcpy(z, iterator1->ipAddress);
                    int send;
                    int errno;
                    struct sockaddr_in somethingda;
                    struct in_addr edo;
                    somethingda.sin_family = AF_INET;
                    inet_aton(z, &edo);
                    somethingda.sin_addr = edo; 
                    somethingda.sin_port = htons(iterator1->portNumber); 

                    if(!flagTimer[iterator1->id-1])
                    {
                        timerCount[iterator1->id-1]++;
                    }
                    if(timerCount[iterator1->id-1] <= 3)
                    {
                        cout <<"SENDING DISTANCE VECTOR UPDATE TO SERVER: "<<iterator1->id <<endl;
                        send = sendto(socket_fd,(void *) mess, messSize, 0, (struct sockaddr*)&(somethingda), sizeof somethingda);
                    }
                    else
                    {
                        cout << "Not sending distance vector to " <<iterator1->id<<endl;
                        costArray[thisElementInfo.id][iterator1->id] = SHRT_MAX_NEW;
                        std::list <NetworkElementsInfo>::iterator iterat99;
                        for(iterat99 = networkElementsList.begin(); iterat99 != networkElementsList.end(); ++iterat99)
                        {
                            if(iterat99->id == iterator1->id)
                                iterat99->isNeighbour = false;
                        }
                        iterator1->cost = SHRT_MAX_NEW;
                    } 
                    cout <<"timerCount[" << iterator1->id-1 <<"] is : " << timerCount[iterator1->id-1] <<endl;
                    cout <<"flagTimer[" << iterator1->id-1 <<"] is : " << flagTimer[iterator1->id-1] <<endl;
                }
                timeOut.tv_sec = atoi(arg4.c_str());
                timeOut.tv_usec = 0 ;
            }
            for(i=0; i<=fdmax; i++)
            {
                if(FD_ISSET(i, &read_fds))
                {
                    if(i == socket_fd)
                    {
                        socklen_t length = sizeof(sourceAddress);
                        recvBytes = recvfrom(i, buf, sizeof(buf), 0, (struct sockaddr*)&sourceAddress, &length);
                        if(recvBytes < 0)
                        {
                            cout << "Error in recvFrom "<<i<<endl;
                        }
                        else if(recvBytes > 0)
                        {
                            inet_ntop(AF_INET, &sourceAddress.sin_addr, ipAddress, INET_ADDRSTRLEN);
			    discard = false;
                            memcpy(&x3,buf,18*sizeof(short int)+6*INET_ADDRSTRLEN);
                            int newId = 0;
                            std::list <NetworkElementsInfo>::iterator iterator20;
                            for(iterator20 = networkElementsList.begin(); iterator20 != networkElementsList.end(); ++iterator20)
                            {
                                stringstream ss1,ss2,ss3,ss4;
                                string s1,s2,s3,s4;
                                ss1<<(int)(x3.ipAddress[0]&0xFF);
                                ss1>> s1;
                                ss2<<(int)(x3.ipAddress[1]&0xFF);
                                ss2>>s2;
                                ss3<<(int)(x3.ipAddress[2]&0xFF);
                                ss3>>s3;
                                ss4<<(int)(x3.ipAddress[3]&0xFF);
                                ss4>>s4;
                                string newIp = s1 +"."+ s2 +"."+ s3+"."+ s4;
                                if(newIp.compare(iterator20->ipAddress) == 0)
                                {
                                    if(iterator20->isNeighbour == false)
                                    {
                                        discard = true;
                                    }
                                    newId = iterator20->id;
                                }
                            }

                                flagTimer[newId-1] = true;
                                timerCount[newId-1] = 0;
                                for(int k=0;k<=4; k++)
                                { 
                                    if(k != newId-1)
                                        flagTimer[k] = false;

                                }
                            if(x3.numOfUpdateFields == 99 )
                            {
                                if(!discard)
                                {
                                    cout<<"TEMPORARY UPDATE RECEIVED" << endl;
                                    costArray[thisElementInfo.id][x3.buffer[0]] = x3.buffer[1]; 
                                    std::list <ThisElementNeighbourInfo>::iterator iteratorOk;
                                    for(iteratorOk = thisElementNeighboursList.begin(); iteratorOk != thisElementNeighboursList.end(); ++iteratorOk)
                                    {
                                        if(iteratorOk->id == x3.buffer[0])
                                        {
                                            if(x3.buffer[1] == SHRT_MAX_NEW) 
                                            {
                                                std::list <NetworkElementsInfo>::iterator iterat;
                                                std::list <ThisElementNeighbourInfo>::iterator iterator1 = thisElementNeighboursList.begin();
                                                while(iterator1 != thisElementNeighboursList.end())
                                                {
                                                    if(iterator1->id == x3.buffer[0])
                                                    {
                                                        thisElementNeighboursList.erase(iterator1++);
                                                        neighbourCount--;
                                                    }
                                                    else
                                                    {
                                                        ++iterator1;
                                                    }
                                                }
                                                costArray[thisElementInfo.id][x3.buffer[0]] = SHRT_MAX_NEW;
                                                for(iterat = networkElementsList.begin(); iterat != networkElementsList.end(); ++iterat)
                                                {
                                                    if(iterat->id == x3.buffer[0])
                                                        iterat->isNeighbour = false;
                                                }
                                            }
                                            else
                                            {
                                                iteratorOk->cost = x3.buffer[1];
                                            }
                                        }
                                    }
                                    updateRoutingTable();
                                }
                            }
                            else
                            {
                                if(!discard)
                                {
                                    packet++;
                                    calculateDistanceVector();
                                }
                            }
                        }
                        else
                        {
                            cout << "Connection closed by neighbour" << endl;
                        }
                    }
                    else
                    {
                        string input;
                        getline(cin, input);
                        cout << "Command given by user is: " << input << endl;

                        bool hasOnlyAlphabets1 = (input.find("update") != string::npos);
                        bool hasOnlyAlphabets2 = (input.find("crash") != string::npos);
                        bool hasOnlyAlphabets3 = (input.find("step") != string::npos);
                        bool hasOnlyAlphabets4 = (input.find("display") != string::npos);
                        bool hasOnlyAlphabets5 = (input.find("disable") != string::npos);
                        bool hasOnlyAlphabets6 = (input.find("packets") != string::npos);
                        if(hasOnlyAlphabets1 || hasOnlyAlphabets2 || hasOnlyAlphabets3 || hasOnlyAlphabets4 || hasOnlyAlphabets5 || hasOnlyAlphabets6)
                        {
                            for(ite =0; ite <=5 ; ite++)
                            {
                                if(input.find(options[ite]) == 0)
                                {
                                    optionGiven = ite;
                                }
                            }
                            string delimiter = " ";
                            size_t pos = 0;
                            string token[6]={};
                            int tr = 1;
                            while((pos = input.find(delimiter)) != string::npos)
                            {
                                token[tr] = input.substr(0,pos);
                                input.erase(0,pos+delimiter.length());
                                token[tr+1] = input.substr(0,100);
                                tr++;
                            }
                            switch(optionGiven)
                            {
                                case 0 : update(token[2], token[3], token[4]);
                                         break;
                                case 1 : step();
                                         break;
                                case 2 : packets();
                                         break;
                                case 3 : disable(token[2]);
                                         break;
                                case 4 : crash();
                                         break;
                                case 5 : display();
                                         break;
                            }
                            input.clear();
                        }
                        else
                        {
                            cout << "Wrong INPUT given" << endl;
                        }
                    }
                }
            }
        }
    }
}
