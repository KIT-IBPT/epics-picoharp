function server_1

pico_init_c;

% t is acquisition time in ms: t = 5000 (ms)
% shif is bin position of the the first maximum: ex shif = 38
% CFDZeroX0 zero crossing of the clock in mV: ex CFDZeroX0 = 10 (between 0
% and 20
% CFDLevel0 level of the clock in mV: ex CFDLevel0 = 350 (between 0
% and 800
%FDZeroX1 zero crossing of the signal in mV: ex CFDZeroX1 = 10 (between 0
% and 20
%CFDLevel1 level of the clock in mV: ex CFDLevel1 = 300 (between 0
% and 800

% Note the initial data should be in the database start!!!!

try
lcaPut('SR-DI-PICO-01:SHIFT',650) % shift to circular shift to dispay the fill pattern nicely
lcaPut('SR-DI-PICO-01:PK_AUTO',1) % bolean: 1 for automatic peak detection

lcaPut('SR-DI-PICO-01:PEAK',45) % initial value for shif
lcaPut('SR-DI-PICO-01:TIME',5000) % t default value in ms
catch
    disp(['cannot lcaGet PVs'])
    pause(10)
end
% CFDZeroX0 =str2num(CFDZeroX0)
% CFDLevel0 =str2num(CFDLevel0)
% CFDZeroX1 =str2num(CFDZeroX1)
% CFDLevel1 =str2num(CFDLevel1)

% dlldemo;

% dlldemo_0(CFDZeroX0,CFDLevel0,CFDZeroX1,CFDLevel1)
dlldemo;


% t = 5000; acquisition time in ms
% shif = 41 bin position of the first peak between 1 and 63
x=0.032*[1:2^16];

buf0 = [] ;
buf1 = [] ;

k = 0;
m = 0;
CHt = [] ;
CHt2 = [] ;

try
    while 1
        k = k + 1;
        m = m + 1;
        try
        t = lcaGet('SR-DI-PICO-01:TIME');
        if isnan(t)
            t=5000;
        end
        % pk_auto is a bolean condition on peak detection automatic or not
        % (used for development) 
        pk_auto = lcaGet('SR-DI-PICO-01:PK_AUTO') ;
        if isnan(pk_auto)
            pk_auto =1;
        end


        catch
            t = 5000;
            pk_auto = 1;
            disp(['cannot lcaGet PVs'])
            disp('set arbitrary pk_auto')
        end
        [buf, co] = picomeasure(t);
   
        try
            frev = lcaGet('LI-RF-MOSC-01:FREQ');
            if isnan(frev)
                frev = 499652713;
            end
                
        catch
            frev = 499652713;
        end

        CountsperTurn = co /t * 1000 / frev * 936 ;
        bufc = double(buf);
        buf0(k,:) = bufc ;
        buf1(m,:) = bufc ;

        max_bin = max(bufc);

        f =bufc(1:58540);
        f0 =sum(buf0(:,1:58540),1);
        f1 =sum(buf1(:,1:58540),1);

        c = round(linspace(0,58540,937));
        c=c(1:end-1);

        if pk_auto==1
            [pk, first_peak_auto] = max(f);
            shif = round(mod(first_peak_auto, 58540/936) );
            if  shif < 5
                disp('<5');
                shif = max(shif,1);
            elseif (shif > 5 & shif < 58)
                shif = shif - 5;
            else
                shif = 53;
            end
            try
                lcaPut('SR-DI-PICO-01:PEAK',shif);
            catch
                disp(['cannot lcaGet PVs'])
                pause(10);
            end
        else
            try
            shif = lcaGet('SR-DI-PICO-01:PEAK')
            if isnan(shif) 
                shif = 53 ;
            end
            catch
                disp(['cannot lcaGet PVs'])
                pause(10)
            end
        end

        try
            charge = lcaGet('SR21C-DI-DCCT-01:CHARGE');
        catch
            charge = 0;
            disp('no DCCT')
        end

        ix=reshape((shif+repmat(1:10,936,1)+repmat(c',1,10))',1,9360);
        S = sum(reshape(f(ix),10,936)) ;
        S0 = sum(reshape(f0(ix),10,936)) ;
        S1 = sum(reshape(f1(ix),10,936)) ;

        total_counts = sum(S);
        tc = co;
        total_counts0 = sum(S0);
        total_counts1 = sum(S1);

        if total_counts==0
            total_counts=1;
        end
        if total_counts0==0
            total_counts0=1;
        end
        if total_counts1==0
            total_counts1=1;
        end
        if charge<0.001
            charge=0.00001;
        end

        Ch = S / total_counts * charge ;
        Ch0 = S0 / total_counts0 * charge ;
        Ch1 = S1 / total_counts1 * charge ;

        CHt(k,:) = Ch0;
        CHt2(m,:)= Ch1;

        fillpico = bufc + 1e-8 ;
        try
            shift_0 =lcaGet('SR-DI-PICO-01:SHIFT');
            if isnan(shift_0)
                shift_0 = 650;
            end
        catch
            disp(['cannot lcaGet PVs']);
            pause(10)
        end
        buckets = circshift( Ch ,[1 -shift_0] ) + 1e-8 ;
        bucketsf = circshift( CHt(k,:) ,[1 -shift_0]) +1e-8 ;
        bucketsf2 = circshift( CHt2(m,:) ,[1 -shift_0]) +1e-8 ;

        pico_test_c;

        if (length(buckets) == 936)
            % put the pvs
            try
                lcaPut('SR-DI-PICO-01:BUCKETS',buckets)
            catch
                 disp(['cannot lcaGet PVs'])
                 pause(10)
            end
        else
            disp('length of buckets is ~=936')
        end

        if (length(fillpico) == 65536)
            % put the pvs
            try
                lcaPut('SR-DI-PICO-01:FILL',fillpico)
            catch
                 disp(['cannot lcaGet PVs'])
                 pause(10)
            end
        else
            disp('length of buckets is ~=65536')
        end

        if (length(bucketsf) == 936)
            % put the pvs
            try
                lcaPut('SR-DI-PICO-01:BUCKETS_60',bucketsf)
            catch
                disp(['cannot lcaGet PVs'])
                pause(10)
            end
        else
            disp('length of bucketsf is ~=936')
        end

        if (length(bucketsf2) == 936)
            % put the pvs
            try
                lcaPut('SR-DI-PICO-01:BUCKETS_180',bucketsf2)
            catch
                 disp(['cannot lcaGet PVs'])
                 pause(10)
            end
        else
            disp('length of bucketsf2 is ~=936')
        end
        
        try 
        lcaPut('SR-DI-PICO-01:MAX_BIN',max_bin)
        lcaPut('SR-DI-PICO-01:FLUX',CountsperTurn)
        lcaPut('SR-DI-PICO-01:COUNTS_5', total_counts )
        lcaPut('SR-DI-PICO-01:COUNTS_60', total_counts0 )
        lcaPut('SR-DI-PICO-01:COUNTS_180', total_counts1 )
        lcaPut('SR-DI-PICO-01:COUNTS_FILL', co )
        catch 
            disp(['cannot lcaGet PVs'])
            pause(10)
        end

        if (k==12)
            k  = 0;
        end

        if (m==36)
            m  = 0;
        end

    end
catch
    closedev;
    datestr(now)
%     [st,bla] = dbstack('-completenames');
disp(lasterr)
end