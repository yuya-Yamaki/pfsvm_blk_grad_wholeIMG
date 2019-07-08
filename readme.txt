松田先生が倉橋先輩に渡したプログラム
loo.sh : cif→cif
looHD.sh : cif→HD
LearnEvalHD_pfsvm.sh : HD→HD

default
cfg : encoder_intra_main_rext.cfg
sample_ratio : 0.01
ga : 0.125
gm : 0.25
c : 2.0

pfsvm_blk_gradのプログラムに対してブロック内部においても輝度勾配を考慮するプログラム．ブロック境界部においては境界線の位置が常に対象画素の上側に来るように設定し，ブロックの隅においてはブロック境界線をまたぐ2候補で勾配計算を行い勾配の大きい方向に回転処理を加えている．