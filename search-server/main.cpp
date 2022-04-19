
#include "../tests/test_Unit.h"
#include "../tests/test_RemoveDocument.h"
#include "../tests/test_MatchDoc.h"
#include "../tests/test_FindTop.h"

using namespace std;

int main(){
    // unit tests
    TestSearchServer();
    // test "test_RemoveDocument.h"
    Test_ProcessQueries();
    Test_ProcessQueriesJoined();
    Test_RemoveDocument();
    //
    Test_MatchDocument();
    //
    Test_FindTop();

    return 0;
}




