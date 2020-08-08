

library(ggplot2)

# SIFT 1M


df <- read.csv("~/University/Thesis/tests/R/final_tests/sift1M_1000_time_quant.csv")
df <- within(df, rm("X"))

recall_keys = c("key1Recall", "key2Recall")
df_rec_mean <- rowMeans(df[recall_keys])
df <- cbind(df, df_rec_mean)
colnames(df)[13] <- "keyRecall"
df$keyMissRate <- 1- df$key1Recall

ggplot(df, aes(x=efSearch, y=time, group=M))+
  geom_line(aes(color=factor(M)))+
  geom_point()+
  labs(color="M")


ggplot(df, aes(x=efSearch, y=keyRecall, group=M))+
  geom_line(aes(color=factor(M)))+
  geom_point()


ggplot(df, aes(x=efSearch, y=quantError, group=M))+
  geom_line(aes(color=factor(M)))+
  geom_point() + 
  geom_hline(yintercept=7.438962, color = "red")




## plot hyperparamter vs time
M_plot <- ggplot(df, aes(x=factor(M), y=time)) + 
  stat_summary(fun="mean", group=1, geom="line") +
  labs(x="max links", y="time (s)")

efS_plot <- ggplot(df, aes(x=factor(efSearch), y=time)) + 
  stat_summary(fun="mean", group=1, geom="line") +
  labs(x="efSearch", y="time (s)") 

gridExtra::grid.arrange(efS_plot, M_plot, nrow = 1) 


## plot hyperparamater vs quantization error

  
#efS_plot_qE <- ggplot(df, aes(x=efSearch, y=quantError)) + 
#  stat_summary(fun="mean", group=1, geom="line") +
#  labs(x="efSearch", y="quantization error") +
  
  
  
#  df %>%       # Specify your table
#    group_by(efSearch, quantError) %>%      # Specify your groups (two variables in your case)
#    summarize(m = mean(quantError))    
  
  
  
#M_plot_qE <- ggplot(df, aes(x=factor(M), y=quantError)) + 
#  stat_summary(fun="mean", group=, geom="line")  + 
#  labs(x="max links", y="quantization error")  

#gridExtra::grid.arrange(efS_plot_qE, M_plot_qE, nrow = 1) 

# SIFT 1M



#  M efConstruction efSearch   nsw       time quantError
df <- read.csv("~/University/Thesis/tests/R/sift1M_test.csv")
df <- within(df, rm("X"))

## plot hyperparamter vs time
#efC_plot <- ggplot(df, aes(x=factor(efConstruction), y=time)) + 
 # stat_summary(fun="mean", group=1, geom="line") + 
#  coord_cartesian(ylim = c(100, 350)) +
#  labs(x="efConstruction", y="time (s)") 
#efS_plot <- ggplot(df, aes(x=factor(efSearch), y=time)) + 
#  stat_summary(fun="mean", group=1, geom="line")  + 
 # coord_cartesian(ylim = c(100, 350)) +
  #labs(x="efSearch", y="time (s)") 
#gridExtra::grid.arrange(efS_plot, efC_plot, nrow = 1) 


## plot hyperparamater vs quantization error
#efS_plot_qE <- ggplot(df, aes(x=factor(efSearch), y=quantError)) + 
#  stat_summary(fun="mean", group=1, geom="line") +
#  labs(x="efSearch", y="quantization error")
#efC_plot_qE <- ggplot(df, aes(x=factor(efConstruction), y=quantError)) + 
#  stat_summary(fun="mean", group=1, geom="line") +
#  labs(x="efConstruction", y="quantization error") 
#M_plot_qE <- ggplot(df, aes(x=factor(M), y=quantError)) + 
#  stat_summary(fun="mean", group=1, geom="line") +
#  labs(x="max links", y="quantization error") 

#gridExtra::grid.arrange(efS_plot_qE, efC_plot_qE, nrow = 1) 



df <- read.csv("~/University/Thesis/tests/R/final_tests/nnodes_test_final_all.csv")

df <- within(df, rm("X"))
df <- df[df$time > 0,] 

orig_df <- read.csv("~/University/Thesis/tests/R/final_tests/orig_gng_time.csv")
orig_df <- within(orig_df, rm("X"))

ggplot(orig_df, aes(x=nnodes, y=time))+
  geom_line(aes(color="GNG")) + geom_point() +
  
  geom_line(mapping = aes(x = df$nnodes, y= df$time, color="Approximate GNG")) + geom_point(x = df$nnodes, y= df$time) +
  ylab("time (s)") +
  xlab("# nodes")+
  labs(color = "Algorithm")

ggplot(df, aes(x=nnodes, y=AvgPathLen))+
  geom_line() + geom_point() +
  ylab("Average Path Length") +
  xlab("# nodes")


df_95 <- read.csv("~/University/Thesis/tests/R/quant500_efS_95.csv")
df_95 <- within(df_95, rm("X"))
df_99 <- read.csv("~/University/Thesis/tests/R/quant500_efS_99.csv")
df_99 <- within(df_99, rm("X"))

df_orig <- df_95[df_95$M == -1,]
df_95 <- df_95[df_95$M > 0,]

ggplot(df_orig, aes(x=nnodes, y=quantError))+
  geom_line(aes(color="GNG")) + geom_point(aes(color="GNG")) +
  
  geom_line(mapping = aes(x = df_95$nnodes, y= df_95$quantError, color="Approx. GNG, search recall = 0.95")) + 
  geom_point(aes(color="Approx. GNG, search recall = 0.95"), x = df_95$nnodes, y= df_95$quantError) +
  
  geom_line(mapping = aes(x = df_99$nnodes, y= df_99$quantError, color="Approx. GNG, search recall = 0.99")) + 
  geom_point(aes(color="Approx. GNG, search recall = 0.99"), x = df_99$nnodes, y= df_99$quantError) +
  
  ylab("Quantization error") +
  xlab("# nodes")+
  labs(color = "Algorithm")





