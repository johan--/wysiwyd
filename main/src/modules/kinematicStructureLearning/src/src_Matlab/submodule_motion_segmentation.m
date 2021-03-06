% ==========================================================================
% Copyright (C) 2014 WYSIWYD Consortium, European Commission FP7 Project ICT-612139
% Authors: Hyung Jin Chang
% email:   (hj.chang@imperial.ac.uk)
% Permission is granted to copy, distribute, and/or modify this program
% under the terms of the GNU General Public License, version 2 or any
% later version published by the Free Software Foundation.
% 
% A copy of the license can be found at
% wysiwyd/license/gpl.txt
% 
% This program is distributed in the hope that it will be useful, but
% WITHOUT ANY WARRANTY; without even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
% Public License for more details
% ==========================================================================

num_frames = size(y,3);
num_points = size(y,2);

motion_ON = true;
motion_seg_plot_ON = true;
motion_seg_connect_plot_ON = true;
result_save_ON = false;

%%
%=========================================================
% motion segmentation
%=========================================================
if motion_ON
    % parameter setting
%         num_seg = 4
%     num_seg = 16
    num_seg = floor(size(y,2)/(8));
%     num_seg = max(ceil(rank(W,20)/4), floor(size(y,2)/(8)))
    %-------------------------------------
    
    num_seg_history = [];
    seg_idx_history = cell(100,1);
    seg_idx_history_idx = 1;
    
    T_i = 200; % the number of iteration     % 150
    T = 3; % the number of trial
    T_c = 20; % convergence test, noise free : 15, noise : 5
    T_r = 15; % reinitialization at T_r
    alpha = 0.9; % decay parameter
    lambda = 2; % voting strength
    
    d = 1;
    converge_check2 = 0;
    
    max_dist = ones(1,num_seg);
    best_overlap = 0;
    
    % y is trajectory, 3 x (the number of points) x (the number of frames)
    % T, T_i, T_c, T_r, alpha, lambda : tuning parameters
    % c : the number of group
    % s : ground truth, (the number of points) x 1
    
    while(1)
        %    final = motion_segmentation(y, T, T_i, T_c, T_r, alpha, lambda, c, s);
        num_seg_history = [num_seg_history;num_seg];        
        final = my_motion_segmentation(y, T, T_i, T_c, T_r, alpha, lambda, num_seg);
        
        % do segmentation
        motion_seg_from_final;
        seg_idx_history{seg_idx_history_idx} = seg_idx;
        seg_idx_history_idx = seg_idx_history_idx + 1;
        motion_seg_num_small_set;
        if num_small_seg == 0
            break;
        else
            num_seg = num_seg - num_small_seg
        end
    end
    
    
    motion_seg_center_calculation;
    
    % segmentation result visualisation
    motion_seg_visualisation;
    
    % connecting segments
    motion_seg_connection;
    
    %////
    %     motion_seg_combining;
    %     % result visualisation
    %     motion_seg_visualisation;
    %     % connecting segments
    %     motion_seg_connection;
    
end
