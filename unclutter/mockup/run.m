function run(N,M,alpha)
  #N = 10; #number of rectangles

  c = zeros(N,2);
  lim = zeros(N,4);
  w = zeros(N,1);

  ##Define the rectangles
  for i = 1:N
    do
      T = sort(rand(2));
    until min(diff(T)) > 0.1
    R{i} = T;
    c(i,:) = com(R{i});
    w(i) =  prod(diff(R{i})); ## The weight, proportional to the area.
    lim(i,1) = c(i,1) - R{i}(1);
    lim(i,2) = 1 + c(i,1) - R{i}(2);
    lim(i,3) = c(i,2) - R{i}(3);
    lim(i,4) = 1 + c(i,2) - R{i}(4);
  endfor
  cinit = c;

  W = w*w';

  close("all")
  ##Plot the initial rectangles
  subplot(2,1,1);
  for i = 1:N
    color{i} = rand(1,3);
    drawrectangle(R{i},color{i});
    xlim([0,1]);
    ylim([0,1]);
    hold on;
  endfor
  hold off;
  title("before");

  ##number of iterations
  #M = 200;

  pathx = zeros(M,N);
  pathy = zeros(M,N);
  for i=1:M
    ## c += ( N*c - repmat(sum(c),N,1) )/(1e2);
    pathx(i,:) = c(:,1)';
    pathy(i,:) = c(:,2)';

    ## pairwise distances cubed, i.e. D(i,j) = |xi - xj|^3
    D = zeros(N); 
    for i=1:N
      for j =i+1:N
	D(i,j) = norm(c(i,:) - c(j,:))^3;
      endfor
    endfor
    D += D'; 

    ##compute the translation vector for each point
    V = zeros(N,2);
    for i=1:N
      V(i,:) = sum((repmat(c(i,:),N-1,1)  \
		    - [c(1:i-1,:);  c(i+1:end,:)]) \
		   ./repmat([D(1:i-1,i); \
			     D(i+1:end,i)],1,2) );
      ##		 .*repmat([W(1:i-1,i); \
      ##			   W(i+1:end,i)],1,2) );
    endfor
    V./repmat(w,1,2);
    c += V/M^alpha;
    
				#clamp to the view window
    c(:,1) = max(lim(:,1), min(lim(:,2),c(:,1)));
    c(:,2) = max(lim(:,3), min(lim(:,4),c(:,2)));
  endfor

  change = c-cinit;

  ##Plot the final rectangles
  subplot(2,1,2)
  for i=1:N
    Rnew{i} = R{i} + repmat(change(i,:),2,1);
    drawrectangle(Rnew{i},color{i});
    xlim([0,1]);
    ylim([0,1]);
    hold on;
  endfor
  hold off;
  title("after")

  ##Show the paths they took
  figure;
  plot(pathx,pathy,'-r');
  hold on;
  plot(cinit(:,1),cinit(:,2),'ob');
  plot(c(:,1),c(:,2),'xg');
  hold off;
  xlim([0,1]);
  ylim([0,1]);
endfunction