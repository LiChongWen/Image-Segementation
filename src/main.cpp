#include <opencv2/opencv.hpp>
#include <exception>
#include <queue>
#include <math.h>

using namespace cv;
using namespace std;
const double INFINIT = 10000000000;
int width;
int height;
struct Pos{
    int x;
    int y;
};
int ab = 5000000;
struct node{
    node* pre = nullptr;
    int flag = -1;
    char pixel = -1;
    int x;
    int y;
    double linkWeight = -1;
    double upWeight = -1;
    double downWeight = -1;
    double leftWeight = -1;
    double rightWeight = -1;
};
void growth(node* graph[], queue<Pos>* activeList, int* path);
void findValidNeigh(queue<Pos>* neighbor, Pos p);
double getWeight(Pos p, Pos next, node* graph[]);
void augmentation(Pos p, Pos next, queue<Pos>* orphanList, node* graph[]);
void clearQueue(queue<node*>* ptList);
void updateFlow(Pos p, Pos next, node* graph[], queue<node*>* ptList, queue<Pos>* orphanList, double min);
void adoption(queue<Pos>* orphanList, node* graph[], queue<Pos>* activeList);
void processP(Pos p, node* graph[], queue<Pos>* activeList, queue<Pos>* orphanList);
int main( int argc, char** argv )
{
    if(argc!=4){
        cout<<"Usage: ../seg input_image initialization_file output_mask"<<endl;
        return -1;
    }
    // Load the input image
    // the image should be a 3 channel image by default but we will double check that in teh seam_carving
    Mat in_image;
    in_image = imread(argv[1]/*, CV_LOAD_IMAGE_COLOR*/);

    if(!in_image.data)
    {
        cout<<"Could not load input image!!!"<<endl;
        return -1;
    }

    if(in_image.channels()!=3){
        cout<<"Image does not have 3 channels!!! "<<in_image.depth()<<endl;
        return -1;
    }
    // the output image
    Mat out_image(in_image.rows, in_image.cols, CV_8UC1);
    //Mat out_image = in_image.clone();
    Mat temp_image = imread(argv[1],0);

    ifstream f(argv[2]);
    if(!f){
        cout<<"Could not load initial mask file!!!"<<endl;
        return -1;
    }

    width = in_image.cols;
    height = in_image.rows;

    node** graph = new node*[height];
    for (int i = 0; i < height; i++) {
        graph[i] = new node[width];
        for (int j = 0; j < width; j++) {
            graph[i][j].pixel = temp_image.at<uchar>(i, j);
            graph[i][j].x = i;
            graph[i][j].y = j;
        }
    }

    queue<Pos> activeList;
    queue<Pos> orphanList;

    int n;
    f>>n;

    for (int i = 0; i < n; i++) {
        int x, y, t;
        f >> x >> y >> t;
        if (x < 0 || x >= width || y < 0 || y >= height){
            cout << "I valid pixel mask!";
        }

        Pos p = {y, x};
        activeList.push(p);
        graph[y][x].linkWeight = INFINIT;
        graph[y][x].flag = t;
    }

    int path[4] = {0,0,0,0};

    while(ab--){

        growth(graph, &activeList, path);
        if(path[0] + path[1] + path[2] + path[3] != 0){
            augmentation(Pos{path[0], path[1]}, Pos{path[2], path[3]}, &orphanList, graph);
            adoption(&orphanList, graph, &activeList);
        } else{
            break;
        }

        path[0] = 0;
        path[1] = 0;
        path[2] = 0;
        path[3] = 0;
    }

    for (int k = 0; k < height; k++) {
        for (int i = 0; i < width; i++) {
            if(graph[k][i].flag == 0){
                out_image.at<uchar>(k, i) = 0;
            } else{
                out_image.at<uchar>(k, i) = 255;
            }
        }
    }

    // write it on disk
    imwrite( argv[3], out_image);

    // also display them both

    namedWindow( "Original image", WINDOW_AUTOSIZE );
    namedWindow( "Show Marked Pixels", WINDOW_AUTOSIZE );
    imshow( "Original image", in_image );
    imshow( "Show Marked Pixels", out_image );
    waitKey(0);
    return 0;
}

void growth(node* graph[], queue<Pos>* activeList, int* path){

    while (!activeList->empty()){
        Pos p = activeList->front();

        if(graph[p.x][p.y].flag != -1){
            queue<Pos> neighborList;
            findValidNeigh(&neighborList, p);

            while(!neighborList.empty()){
                Pos next = neighborList.front();

                double gap = getWeight(p, next, graph);
                if(gap > 0){
                    if(graph[next.x][next.y].flag == -1){
                        graph[next.x][next.y].flag = graph[p.x][p.y].flag;
                        graph[next.x][next.y].linkWeight = gap;
                        graph[next.x][next.y].pre = &graph[p.x][p.y];
                        activeList->push(next);
                    }
                    if(graph[next.x][next.y].flag != -1 && graph[next.x][next.y].flag != graph[p.x][p.y].flag){
                        path[0] = p.x;
                        path[1] = p.y;
                        path[2] = next.x;
                        path[3] = next.y;
                        return;
                    }
                }
                neighborList.pop();
            }
        }

        activeList->pop();
    }

}

void findValidNeigh(queue<Pos>* neighbor, Pos p){
    int x = p.x;
    int y = p.y;
    if(x == 0){
        neighbor->push(Pos{x + 1, y});
        if(y != 0){
            neighbor->push(Pos{x, y - 1});
        } else if (y != width - 1){
            neighbor->push(Pos{x, y + 1});
            return;
        }
    } else if (x == height - 1){
        neighbor->push(Pos{x - 1, y});
        if(y != 0){
            neighbor->push(Pos{x, y - 1});
        }
        if (y != width - 1){
            neighbor->push(Pos{x, y + 1});
            return;
        }
    } else{
        neighbor->push(Pos{x + 1, y});
        neighbor->push(Pos{x - 1, y});
        if(y != width - 1){
            neighbor->push(Pos{x, y + 1});
        }
        if(y != 0){
            neighbor->push(Pos{x, y - 1});
        }
    }
}

double getWeight(Pos p, Pos next, node* graph[]){

    double pPixel = graph[p.x][p.y].pixel;
    double nextP = graph[next.x][next.y].pixel;

    int diff = (p.x - next.x )* width + p.y - next.y;
    switch(diff){
        case 1:
            if(graph[next.x][next.y].rightWeight == -1) {
                double gap = INFINIT;
                if(pPixel != nextP){
                    double temp = pow(nextP - pPixel, 2)/2;
                    gap = INFINIT / temp;
                }
                graph[next.x][next.y].rightWeight = gap;
                if(graph[p.x][p.y].leftWeight == -1){
                    graph[p.x][p.y].leftWeight = gap;
                }
                return gap;
            }else {
                return graph[next.x][next.y].rightWeight;
            }

        case -1:
            if(graph[next.x][next.y].leftWeight == -1) {
                double gap = INFINIT;
                if(pPixel != nextP){
                    double temp = pow(nextP - pPixel, 2)/2;
                    gap = INFINIT / temp;
                }
                graph[next.x][next.y].leftWeight = gap;
                if(graph[p.x][p.y].rightWeight == -1){
                    graph[p.x][p.y].rightWeight = gap;
                }
                return gap;
            } else{
                return graph[next.x][next.y].leftWeight;
            }
        default:
            double gap = INFINIT;
            if(diff == width){
                if(graph[next.x][next.y].downWeight == -1) {
                    if(pPixel != nextP){
                        double temp = pow(nextP - pPixel, 2)/2;
                        gap = INFINIT / temp;
                    }
                    graph[next.x][next.y].downWeight = gap;
                    if(graph[p.x][p.y].upWeight == -1){
                        graph[p.x][p.y].upWeight = gap;
                    }
                    return gap;
                } else{
                    return graph[next.x][next.y].downWeight;
                }
            } else{
                if(graph[next.x][next.y].upWeight == -1) {
                    if(pPixel != nextP){
                        double temp = pow(nextP - pPixel, 2)/2;
                        gap = INFINIT / temp;
                    }
                    graph[next.x][next.y].upWeight = gap;
                    if(graph[p.x][p.y].downWeight == -1){
                        graph[p.x][p.y].downWeight = gap;
                    }
                    return gap;
                } else{
                    return graph[next.x][next.y].upWeight;
                }
            }

    }
}
void augmentation(Pos p, Pos next, queue<Pos>* orphanList, node* graph[]){
    double min = getWeight(p, next, graph);
    double temp = min;
    node* pt = &graph[p.x][p.y];
    queue<node*> ptList;

    while( pt != nullptr){
        if(pt->linkWeight < min){
            clearQueue(&ptList);
            min = pt->linkWeight;
            ptList.push(pt);
        } else if(pt->linkWeight == min && pt->linkWeight != INFINIT){
            ptList.push(pt);
        }

        if(pt->pre == nullptr && pt->flag != graph[next.x][next.y].flag){
            pt = &graph[next.x][next.y];
        } else {
            pt = pt->pre;
        }

    }

    if(min == temp && min != INFINIT){
        long diff = &graph[next.x][next.y] - &graph[p.x][p.y];
        switch(diff){
            case 1:     graph[next.x][next.y].leftWeight -= min;
                graph[p.x][p.y].rightWeight -= min;
                break;
            case -1:    graph[next.x][next.y].rightWeight -= min;
                graph[p.x][p.y].leftWeight -= min;
                break;
            default:
                if(diff == width){
                    graph[next.x][next.y].upWeight -= min;
                    graph[p.x][p.y].downWeight -= min;
                } else{
                    graph[next.x][next.y].downWeight -= min;
                    graph[p.x][p.y].upWeight -= min;
                }
        }
    }

    updateFlow(p, next, graph, &ptList, orphanList, min);
}
void clearQueue(queue<node*>* ptList){
    while(!ptList->empty()){
        ptList->pop();
    }
}
void updateFlow(Pos p, Pos next, node* graph[], queue<node*>* ptList, queue<Pos>* orphanList, double min){

    node* pt = &graph[p.x][p.y];

    while(pt != nullptr){
        if(pt->linkWeight != INFINIT){
            pt->linkWeight -= min;

            if(pt->pre != nullptr){
                long diff = pt - pt->pre;
                switch(diff){
                    case 1:     pt->leftWeight = pt->linkWeight;
                        pt->pre->rightWeight -= min;
                        break;
                    case -1:    pt->rightWeight = pt->linkWeight;
                        pt->pre->leftWeight -= min;
                        break;
                    default:
                        if(diff == width){
                            pt->upWeight = pt->linkWeight;
                            pt->pre->downWeight -= min;
                        } else{
                            pt->downWeight = pt->linkWeight;
                            pt->pre->upWeight -= min;
                        }
                }
            }

        }

        if(pt->pre == nullptr && pt->flag != graph[next.x][next.y].flag){
            pt = &graph[next.x][next.y];
        } else{
            pt = pt->pre;
        }
    }

/*    if(ptList->empty()){
        cout << p.x << " " << p.y << " ptList empty" << endl;
    }*/
    while(!ptList->empty()){
        pt = ptList->front();
        //cout << pt->x << " " << pt->y << " " << pt->linkWeight << endl;

        if(pt->pre != nullptr){
            if(pt->flag == pt->pre->flag){
                pt->pre = nullptr;
                orphanList->push(Pos{pt->x, pt->y});
            }
        }
        ptList->pop();
    }
}
void adoption(queue<Pos>* orphanList, node* graph[], queue<Pos>* activeList){
    while(!orphanList->empty()){
        Pos p = orphanList->front();
        orphanList->pop();

        processP(p, graph, activeList, orphanList);
    }
}
void processP(Pos p, node* graph[], queue<Pos>* activeList, queue<Pos>* orphanList) {
    queue<Pos> neighbors;
    findValidNeigh(&neighbors, p);
    while (!neighbors.empty()) {
        Pos pre = neighbors.front();

        if (graph[pre.x][pre.y].flag != -1) {
            if (graph[pre.x][pre.y].flag == graph[p.x][p.y].flag) {
                if (getWeight(pre, p, graph) > 0) {
                    activeList->push(pre);
                }
                if (graph[pre.x][pre.y].pre == &graph[p.x][p.y]) {
                    graph[pre.x][pre.y].pre = nullptr;
                    orphanList->push(pre);
                }
            }
        }

        neighbors.pop();
    }
    graph[p.x][p.y].flag = -1;

}