#include"blastneuron/resampling.h"
#include "blastneuron/sort_swc.h"
#define NTDIS(a,b) (sqrt(((a).x-(b).x)*((a).x-(b).x)+((a).y-(b).y)*((a).y-(b).y)+((a).z-(b).z)*((a).z-(b).z)))

bool prune_branch(NeuronTree &nt, NeuronTree & result, double prune_size)
{

    V3DLONG siz = nt.listNeuron.size();
    vector<V3DLONG> branches(siz,0); //number of branches on the pnt: 0-tip, 1-internal, >=2-branch
    for (V3DLONG i=0;i<siz;i++)
    {
        if (nt.listNeuron[i].pn<0) continue;
        V3DLONG pid = nt.hashNeuron.value(nt.listNeuron[i].pn);
        branches[pid]++;
    }

    double diameter = calculate_diameter(nt, branches);
    printf("diameter=%.3f\n",diameter);

    if (prune_size  == -1 ){
        double thres = 0.05;
        prune_size = diameter * thres;
    }
    //calculate the shortest edge starting from each tip point
    vector<bool> to_prune(siz, false);
    for (V3DLONG i=0;i<siz;i++)
    {
        if (branches[i]!=0) continue;
        //only consider tip points
        vector<V3DLONG> segment;
        double edge_length = 0;
        V3DLONG cur = i;
        V3DLONG pid;
        do
        {
            NeuronSWC s = nt.listNeuron[cur];
            segment.push_back(cur);
            pid = nt.hashNeuron.value(s.pn);
            edge_length += NTDIS(s, nt.listNeuron[pid]);
            cur = pid;
        }
        while (branches[pid]==1 && pid>0);
        if (pid<0)
        {
            printf("The input tree has only 1 root point. Please check.\n");
            return false;
        }
        if (edge_length < prune_size)
        {
            for (int j=0;j<segment.size();j++)
                to_prune[segment[j]] = true;
        }
    }



    //prune branches
    result.listNeuron.clear();
    result.hashNeuron.clear();
    for (V3DLONG i=0;i<siz;i++)
    {
        if (!to_prune[i])
        {
            NeuronSWC s = nt.listNeuron[i];
            result.listNeuron.append(s);
            result.hashNeuron.insert(nt.listNeuron[i].n, result.listNeuron.size()-1);
        }
    }

    return true;
}

double calculate_diameter(NeuronTree nt, vector<V3DLONG> branches)
{
    V3DLONG siz = nt.listNeuron.size();
    vector<vector<double> > longest_path(siz, vector<double>(2,0));//the 1st and 2nd longest path to each node in a rooted tree
    vector<V3DLONG> chd(siz, -1);//immediate child of the current longest path
    for (V3DLONG i=0;i<siz;i++)
    {
        if (branches[i]!=0) continue;
        V3DLONG cur = i;
        V3DLONG pid;
        do
        {
            NeuronSWC s = nt.listNeuron[cur];
            pid = nt.hashNeuron.value(s.pn);
            double dist = NTDIS(s, nt.listNeuron[pid]) + longest_path[cur][0];
            if (dist>longest_path[pid][0])
            {
                chd[pid] = cur;
                longest_path[pid][0] = dist;
            }
            else if (dist>longest_path[pid][1] && chd[pid]!=cur)
                longest_path[pid][1] = dist;
            cur = pid;
        }
        while (branches[cur]!=0 && pid>0);
    }

    double diam = -1;
    for (V3DLONG i=0;i<siz;i++)
    {
        if (longest_path[i][0] + longest_path[i][1]>diam)
            diam = longest_path[i][0]+longest_path[i][1];
    }
    return diam;
}

QList<NeuronSWC> prune_long_alignment(QList<NeuronSWC> &neuron,double thres)
{
    QList<NeuronSWC> result;
    V3DLONG siz = neuron.size();
    double dist;
    for(V3DLONG i=0; i<siz-1;i=i+2)
    {
        dist = sqrt((neuron[i].x-neuron[i+1].x)*(neuron[i].x-neuron[i+1].x)
                +(neuron[i].y-neuron[i+1].y)*(neuron[i].y-neuron[i+1].y)
                +(neuron[i].z-neuron[i+1].z)*(neuron[i].z-neuron[i+1].z));
        if(dist>=0 && dist <=thres){result.push_back(neuron[i]);result.push_back(neuron[i+1]);}
    }
    return result;
}

bool sort_with_standard(QList<NeuronSWC>  &neuron1, QList<NeuronSWC> & neuron2,QList<NeuronSWC>  &result)
{
    V3DLONG siz = neuron1.size();
    V3DLONG root_id = 1;
    double dist;
    if (siz==0) return false;
    double min_dist = sqrt((neuron1[0].x-neuron2[0].x)*(neuron1[0].x-neuron2[0].x)
         +(neuron1[0].y-neuron2[0].y)*(neuron1[0].y-neuron2[0].y)
         +(neuron1[0].z-neuron2[0].z)*(neuron1[0].z-neuron2[0].z));
    for(V3DLONG i=0; i<siz; i++)
    {
         dist = sqrt((neuron1[i].x-neuron2[0].x)*(neuron1[i].x-neuron2[0].x)
                +(neuron1[i].y-neuron2[0].y)*(neuron1[i].y-neuron2[0].y)
                +(neuron1[i].z-neuron2[0].z)*(neuron1[i].z-neuron2[0].z));
         if(min_dist > dist) {min_dist = dist; root_id = i+1;}
    }
    cout<<"min_dist = "<< min_dist <<endl;
    cout<<"root_id = " << root_id <<endl;

    //sort_swc process
    double thres = 10000;
    if(!Sort_auto_SWC(neuron1,result,root_id,thres))
    {
        cout<<"Error in sorting swc"<<endl;
        return false;
    }
    return true;
}

void resample_path(Segment * seg, double step)
{
    char c;
    Segment seg_r;
    double path_length = 0;
    Point* start = seg->at(0);
    Point* seg_par = seg->back()->p;
    V3DLONG iter_old = 0;
    seg_r.push_back(start);
    while (iter_old < seg->size() && start && start->p)
    {
        path_length += DISTP(start,start->p);
        if (path_length<=seg_r.size()*step)
        {
            start = start->p;
            iter_old++;
        }
        else//a new point should be created
        {
            path_length -= DISTP(start,start->p);
            Point* pt = new Point;
            double rate = (seg_r.size()*step-path_length)/(DISTP(start,start->p));
            pt->x = start->x + rate*(start->p->x-start->x);
            pt->y = start->y + rate*(start->p->y-start->y);
            pt->z = start->z + rate*(start->p->z-start->z);
            pt->r = start->r*(1-rate) + start->p->r*rate;//intepolate the radius
            pt->p = start->p;
            if (rate<0.5) pt->type = start->type;
            else pt->type = start->p->type;
            seg_r.back()->p = pt;
            seg_r.push_back(pt);
            path_length += DISTP(start,pt);
            start = pt;
        }
    }
    seg_r.back()->p = seg_par;
    for (V3DLONG i=0;i<seg->size();i++)
        if (!seg->at(i)) {delete seg->at(i); seg->at(i) = NULL;}
    *seg = seg_r;
}

NeuronTree resample(NeuronTree input, double step)
{
    NeuronTree result;
    V3DLONG siz = input.listNeuron.size();
    Tree tree;
    for (V3DLONG i=0;i<siz;i++)
    {
        NeuronSWC s = input.listNeuron[i];
        Point* pt = new Point;
        pt->x = s.x;
        pt->y = s.y;
        pt->z = s.z;
        pt->r = s.r;
        pt ->type = s.type;
        pt->p = NULL;
        pt->childNum = 0;
        tree.push_back(pt);
    }
    for (V3DLONG i=0;i<siz;i++)
    {
        if (input.listNeuron[i].pn<0) continue;
        V3DLONG pid = input.hashNeuron.value(input.listNeuron[i].pn);
        tree[i]->p = tree[pid];
        tree[pid]->childNum++;
    }
//	printf("tree constructed.\n");
    vector<Segment*> seg_list;
    for (V3DLONG i=0;i<siz;i++)
    {
        if (tree[i]->childNum!=1)//tip or branch point
        {
            Segment* seg = new Segment;
            Point* cur = tree[i];
            do
            {
                seg->push_back(cur);
                cur = cur->p;
            }
            while(cur && cur->childNum==1);
            seg_list.push_back(seg);
        }
    }
//	printf("segment list constructed.\n");
    for (V3DLONG i=0;i<seg_list.size();i++)
    {
        resample_path(seg_list[i], step);
    }

//	printf("resample done.\n");
    tree.clear();
    map<Point*, V3DLONG> index_map;
    for (V3DLONG i=0;i<seg_list.size();i++)
        for (V3DLONG j=0;j<seg_list[i]->size();j++)
        {
            tree.push_back(seg_list[i]->at(j));
            index_map.insert(pair<Point*, V3DLONG>(seg_list[i]->at(j), tree.size()-1));
        }
    for (V3DLONG i=0;i<tree.size();i++)
    {
        NeuronSWC S;
        Point* p = tree[i];
        S.n = i+1;
        if (p->p==NULL) S.pn = -1;
        else
            S.pn = index_map[p->p]+1;
        if (p->p==p) printf("There is loop in the tree!\n");
        S.x = p->x;
        S.y = p->y;
        S.z = p->z;
        S.r = p->r;
        S.type = p->type;
        result.listNeuron.push_back(S);
    }
    for (V3DLONG i=0;i<tree.size();i++)
    {
        if (tree[i]) {delete tree[i]; tree[i]=NULL;}
    }
    for (V3DLONG j=0;j<seg_list.size();j++)
        if (seg_list[j]) {delete seg_list[j]; seg_list[j] = NULL;}
    for (V3DLONG i=0;i<result.listNeuron.size();i++)
        result.hashNeuron.insert(result.listNeuron[i].n, i);
    return result;
}

