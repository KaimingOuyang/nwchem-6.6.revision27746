ckbn Kiran Bhaskaran-Nair modified from tce_print_x2.F 
ckbn Has to be debugged
      subroutine tce_print_eax2(d_r2,k_r2_offset,r2_thresh,irrep)
c
c $Id: tce_print_eax2.F 25748 2014-06-08 07:53:05Z d3y133 $
c
c Prints the X2 tensor
c
      implicit none
#include "global.fh"
#include "mafdecls.fh"
#include "sym.fh"
#include "util.fh"
#include "stdio.fh"
#include "errquit.fh"
#include "tce.fh"
#include "tce_main.fh"
      integer d_r2
      integer p1b
      integer p2b
      integer h3b
      integer h4b
      integer p1
      integer p2
      integer h3
      integer h4
      integer k_r2_offset
      integer size
      integer l_r2,k_r2
      integer i
      integer irrep
      logical nodezero
      double precision r2_thresh
      character*4 irrep_p1,irrep_p2,irrep_h3,irrep_h4
      character*5 spinnames(2),spin_p1,spin_p2,spin_h3,spin_h4
      data spinnames/'alpha','beta '/
c
      nodezero = (ga_nodeid().eq.0)
      if (nodezero) write(LuOut,9000) 'Doubles contributions EA-EOMCCSD'
      do p1b = noab+1,noab+nvab
        do p2b = p1b,noab+nvab
          do h3b = 1,noab
            do h4b = 1,1
              if (int_mb(k_spin+p1b-1)+int_mb(k_spin+p2b-1) 
     1          .eq. int_mb(k_spin+h3b-1)+ ea_unused_spin) then
              if ((.not.restricted).or.
     1          (int_mb(k_spin+p1b-1)+int_mb(k_spin+p2b-1)+
     2          int_mb(k_spin+h3b-1)+ ea_unused_spin .ne.8)) then
              if (ieor(int_mb(k_sym+p1b-1),ieor(int_mb(k_sym+p2b-1),
     1          ieor(int_mb(k_sym+h3b-1),ea_unused_sym)))
     2          .eq. irrep) then
                size = int_mb(k_range+p1b-1) * int_mb(k_range+p2b-1)
     1               * int_mb(k_range+h3b-1) * 1
                if (.not.ma_push_get(mt_dbl,size,'r2',l_r2,k_r2))
     1            call errquit('tce_print_x2: MA problem',0,MA_ERR)
c                write(LuOut,*) "I am here 1"
c                call util_flush(LuOut)
                call get_hash_block(d_r2,dbl_mb(k_r2),size,
     1            int_mb(k_r2_offset),((((p1b-noab-1)*nvab+p2b-noab-1)
     2            *noab+h3b-1)*noab+h4b-1))
c                write(LuOut,*) "I am here 2"
c                call util_flush(LuOut)
                i = 0
                do p1 = 1,int_mb(k_range+p1b-1)
                  do p2 = 1,int_mb(k_range+p2b-1)
                    do h3 = 1,int_mb(k_range+h3b-1)
ckbn                      do h4 = 1,int_mb(k_range+h4b-1)
                      do h4 = 1,1
              if (dabs(dbl_mb(k_r2+i-1)).gt.r2_thresh) then
                call sym_irrepname(geom,int_mb(k_sym+p1b-1)+1,irrep_p1)
                call sym_irrepname(geom,int_mb(k_sym+p2b-1)+1,irrep_p2)
                call sym_irrepname(geom,int_mb(k_sym+h3b-1)+1,irrep_h3)
                spin_p1 = spinnames(int_mb(k_spin+p1b-1))
                spin_p2 = spinnames(int_mb(k_spin+p2b-1))
                spin_h3 = spinnames(int_mb(k_spin+h3b-1))
                if (nodezero) write(LuOut,9010)
     1            (int_mb(k_mo_index+int_mb(k_offset+p1b-1)+p1-1)+
     1             2-int_mb(k_spin+p1b-1))/2,
     1            irrep_p1,spin_p1,
     2            (int_mb(k_mo_index+int_mb(k_offset+p2b-1)+p2-1)+
     2             2-int_mb(k_spin+p2b-1))/2,
     2            irrep_p2,spin_p2,
     3            (int_mb(k_mo_index+int_mb(k_offset+h3b-1)+h3-1)+
     3             2-int_mb(k_spin+h3b-1))/2,
     3            irrep_h3,spin_h3,
     5            dbl_mb(k_r2+i-1)
              endif
                      enddo
                    enddo
                  enddo
                enddo
                if (.not.ma_pop_stack(l_r2))
     1            call errquit('tce_print_x2: MA problem',1,MA_ERR)
              endif
              endif
              endif
            enddo
          enddo
        enddo
      enddo
      call ga_sync()
 9000 format(/,1x,a)
 9010 format(1x,i5,a4,'(',a5,') ',i5,a4,'(',a5,') --- ',
     1          i5,a4,'(',a5,') ',f20.10)
      return
      end
