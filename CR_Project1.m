format longG;

TimeSlots=1:30000;
Bands=1:100;
SUs=1:10;

modes=["k-o" "g--^" "b-.x" "r:square"];

%__________________________________________________________________________

PU_Activation_Probability = ["0.2" "0.4" "0.5" "0.6"];
PU_Activation_Probability_Naming = ["0_2" "0_4" "0_5" "0_6"];
Condition = ["Deterministic" "DTMC"];
Method = ["Local" "Majority"];
Distribution = ["PerSlot" "PerBand"];

Condition_File = ["deterministic" "Markov"];
Method_File = ["local" "majority"];

Calculations_Per_Slot=["Y27" "AvgY" "U27" "AvgU" "S7" "AvgS" "C7" "AvgC"... 
    "F7" "AvgF" "D7" "AvgD" "I27" "AvgI"];
Calculations_Per_Band_for_Band=["AvgY" "AvgU" 'N' 'N' 'N' 'N' "AvgI"];
Calculations_Per_Band_for_SU=[ 'N' 'N' "AvgS" "AvgC" "AvgF" "AvgD"];

Calculations_Per_Slot_File=["Y_27" "Y" "U_27" "U" "S_7" "S" "C_7" "C"... 
    "F_7" "F" "D_7" "D" "I_27" "I"];
Calculations_Per_Band_for_Band_File=["Y" "U" 'N' 'N' 'N' 'N' "I"];
Calculations_Per_Band_for_SU_File=[ 'N' 'N' "S" "C" "F" "D"];

Calculations_Per_Slot_fig=["Channel Throughput for band 27" "Average Channel Throughput" "Channel Utilization for band 27" "Average Channel Utilization" "SU Successful Transmission SU7" "Average SU Successful Transmission" "SU Collisions SU7" "Average SU Collisions"... 
    "SU False Alarm Count SU7" "Average SU False Alarm Count" "SU Misdetection Count SU7" "Average SU Misdetection Count" "Channel Interference for band 27" "Average Channel Interference"];
Calculations_Per_Band_for_Band_fig=["Average Channel Throughput" "Average Channel Utilization" 'N' 'N' 'N' 'N' "Average Channel Interference"];
Calculations_Per_Band_for_SU_fig=[ 'N' 'N' "Average SU Successful Transmission" "Average SU Collisions" "Average SU False Alarm Count" "Average SU Misdetection Count"];

Names = strings(32,1); 
idx = 1;

for i = 1:4
    for j = 1:2
        for u = 1:2
            for s = 1:2
                Names(idx) = PU_Activation_Probability(i) +'_'+ Condition(j) +'_'+ Method(u)+'_'+ Distribution(s)+'_'+"Values.csv";
                idx = idx + 1;
            end
        end
    end
end

NamesMatrix = reshape(Names, 8, 4)'; 

allData = cell(4, 8); 

for l =1:4
    for z =1:8
        allData{l,z}=csvread(NamesMatrix(l,z));
    end
end

clear c d  i j l rawName varName s u idx z;

%__________________________________________________________________________

for i=1:2
    for j=1:2
        File_Name = Condition_File(i)+'_'+Method_File(j)+'_'+"sensing";

        if ~exist(File_Name,'dir')
            mkdir(File_Name);
        end

    end
end

clear i j;

%__________________________________________________________________________

idx=1;

CalcS=strings(1,224);

for i=1:14
    for j=1:2
        for s=1:2
            for u=1:4
                CalcS(idx)=Calculations_Per_Slot(i)+'_'+Condition(j)+'_'+Method(s)+'_'+Distribution(1)+'_'+PU_Activation_Probability_Naming(u);
                assignin('base', CalcS(idx), 0);
                idx = idx + 1;
            end
        end
    end
end

clear i u j s;

idx=1;

xx=cell(1,16*14);

for i=1:14
    for j=1:2:7
        for u=1:4
            x=allData{u,j}(:,i)';
            xx{idx}=x;
            assignin('base' , CalcS(idx), allData{u,j}(:,i));
            idx = idx + 1;
        end
    end
end

idx=1;
Pidx=1;

FigNamesS=strings(1,56);
FigNamesS_=strings(1,56);
PicNamesS=strings(1,56);
PicNamesS_=strings(1,56);

for c=1:14
    for z=1:2
        for t=1:2
            FigNamesS(Pidx)=Calculations_Per_Slot(c)+' '+Condition(z)+' '+Method(t);
            FigNamesS_(Pidx)=Calculations_Per_Slot_fig(c)+' '+Condition(z)+' '+Method(t);
            figure("Name",FigNamesS(Pidx));
            hold on
            for s=1:4
                plot(TimeSlots,xx{1,idx},modes(s),"LineWidth",2);
                idx=idx+1;
            end
            xlabel("TimeSlot");
            ylabel(FigNamesS_(Pidx))
            legend("PU Active at 0.2","PU Active at 0.4","PU Active at 0.5","PU Active at 0.6");
            hold off
            
            if mod(c,2)==0
            PicNamesS(Pidx)=Calculations_Per_Slot_File(c)+'_'+'t'+".png";
            PicNamesS_(Pidx)=Calculations_Per_Slot_File(c)+'_'+'t'+".fig";
            else
            PicNamesS(Pidx)=Calculations_Per_Slot_File(c)+".png";
            PicNamesS_(Pidx)=Calculations_Per_Slot_File(c)+".fig";
            end

            File_Name=Condition_File(z)+'_'+Method_File(t)+'_'+"sensing";
            fullfile(File_Name);

            saving_Pic_Names_png=fullfile(File_Name,PicNamesS(Pidx));
            saving_Pic_Names_fig=fullfile(File_Name,PicNamesS_(Pidx));
            saveas(gcf, saving_Pic_Names_png);
            saveas(gcf, saving_Pic_Names_fig);
            
            close;

            Pidx=Pidx+1;
        end
    end
end

%__________________________________________________________________________

idx=1;

CalcB=strings(1,48);

for i=[1 2 7]
    for j=1:2
        for s=1:2
            for u=1:4
                CalcB(idx)=Calculations_Per_Band_for_Band(i)+'_'+Condition(j)+'_'+Method(s)+'_'+Distribution(2)+'_'+PU_Activation_Probability_Naming(u);
                assignin('base', CalcB(idx), 0);
                idx = idx + 1;
            end
        end
    end
end

clear idx i u j s;

idx=1;

yy=cell(1,16*3);

for i=[1 2 7]
    for j=2:2:8
        for u=1:4
            y=allData{u,j}(i,1:100);
            yy{idx}= y;
            assignin('base' , CalcB(idx), allData{u,j}(i,1:100));
            idx = idx + 1;
        end
    end
end

idx=1;
Pidx=1;

FigNamesB=strings(1,48);
FigNamesB_=strings(1,48);
PicNamesS=strings(1,48);
PicNamesS_=strings(1,48);

for c=[1 2 7]
    for z=1:2
        for t=1:2
            FigNamesB(Pidx)=Calculations_Per_Band_for_Band(c)+' '+Condition(z)+' '+Method(t);
            FigNamesB_(Pidx)=Calculations_Per_Band_for_Band_fig(c)+' '+Condition(z)+' '+Method(t);
            figure("Name",FigNamesB(Pidx));
            hold on
            for s=1:4
                plot(Bands,yy{1,idx},modes(s),"LineWidth",2);
                idx=idx+1;
            end
            xlabel("Bands");
            ylabel(FigNamesB_(Pidx))
            legend("PU Active at 0.2","PU Active at 0.4","PU Active at 0.5","PU Active at 0.6");
            hold off
            
            PicNamesS(Pidx)=Calculations_Per_Band_for_Band_File(c)+'_'+'m'+".png";
            PicNamesS_(Pidx)=Calculations_Per_Band_for_Band_File(c)+'_'+'m'+".fig";

            File_Name=Condition_File(z)+'_'+Method_File(t)+'_'+"sensing";
            fullfile(File_Name);

            saving_Pic_Names_png=fullfile(File_Name,PicNamesS(Pidx));
            saving_Pic_Names_fig=fullfile(File_Name,PicNamesS_(Pidx));
            saveas(gcf, saving_Pic_Names_png);
            saveas(gcf, saving_Pic_Names_fig);
            
            close;
            Pidx=Pidx+1;

        end
    end
end

%__________________________________________________________________________________________________________________________________

idx=1;

CalcSU=strings(1,64);

for i=3:6
    for j=1:2
        for s=1:2
            for u=1:4
                CalcSU(idx)=Calculations_Per_Band_for_SU(i)+'_'+Condition(j)+'_'+Method(s)+'_'+Distribution(2)+'_'+PU_Activation_Probability_Naming(u);
                assignin('base', CalcSU(idx), 0);
                idx = idx + 1;
            end
        end
    end
end

clear idx i u j s;

idx=1;

zz=cell(1,16*4);

for i=3:6
    for j=2:2:8
        for u=1:4
            z=allData{u,j}(i,1:10);
            zz{idx}= z;
            assignin('base' , CalcSU(idx), allData{u,j}(i,1:10));
            idx = idx + 1;
        end
    end
end

idx=1;
Pidx=1;

FigNamesSU=strings(1,64);
FigNamesSU_=strings(1,64);
PicNamesS=strings(1,64);
PicNamesS_=strings(1,64);

for c=3:6
    for z=1:2
        for t=1:2
            FigNamesSU(Pidx)=Calculations_Per_Band_for_SU(c)+' '+Condition(z)+' '+Method(t);
            FigNamesSU_(Pidx)=Calculations_Per_Band_for_SU_fig(c)+' '+Condition(z)+' '+Method(t);
            figure("Name",FigNamesSU(Pidx));
            hold on
            for s=1:4
                plot(SUs,zz{1,idx},modes(s),"LineWidth",2);
                idx=idx+1;
            end
            xlabel("Secondary User");
            ylabel(FigNamesSU_(Pidx))
            legend("PU Active at 0.2","PU Active at 0.4","PU Active at 0.5","PU Active at 0.6");
            hold off
            PicNamesS(Pidx)=Calculations_Per_Band_for_SU_File(c)+'_'+'n'+".png";
            PicNamesS_(Pidx)=Calculations_Per_Band_for_SU_File(c)+'_'+'n'+".fig";

            File_Name=Condition_File(z)+'_'+Method_File(t)+'_'+"sensing";
            fullfile(File_Name);

            saving_Pic_Names_png=fullfile(File_Name,PicNamesS(Pidx));
            saving_Pic_Names_fig=fullfile(File_Name,PicNamesS_(Pidx));
            saveas(gcf, saving_Pic_Names_png);
            saveas(gcf, saving_Pic_Names_fig);
            
            close;

            Pidx = Pidx + 1;
        end
    end
end

%__________________________________________________________________________

clear Pidx idx i j u c s Names t x y z yy xx zz ;