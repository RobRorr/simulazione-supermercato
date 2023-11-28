#!/bin/bash

if [ $# -ne 1 ]; then       #Errore se passato più di un argomento
    echo SH ERROR INPUT
    exit -1;
fi

tx=$1
ex=${tx##*.}
if [ x$ex != x"log" ]; then #Verifica estensione .log
   echo SH ERROR EXTENSION
   exit 0;
fi

d_np=() #Numero Prodotti venduti                 #array casse
d_cl=() #Numero Clienti
d_tt=() #Tempo totale apertura
d_tm=() #Tempo medio di servizio
d_ch=() #Numero chiusure
cnt=-1                                           #contatore cassa

for f ; do                                       #Creazione output da .log
    exec 3<$f
    
    while read -u 3 line; do                     #Caso cliente
	read -r -a elem <<< $line
	case ${elem[0]} in
	    ("Cl:")
		echo "|id cl:"${elem[1]}"|NumPr:"${elem[4]}"|Tempo_Tot:"${elem[2]}"|Tempo_Coda:"${elem[3]}"|";;
	esac
	case ${elem[0]} in                       #Caso cassa
	    ("Cas:")
		(( cnt += 1 ))
		d_cl[${elem[1]}]=${elem[2]}
		d_tt[${elem[1]}]=${elem[3]}
		d_ch[${elem[1]}]=${elem[4]}
		d_tm[${elem[1]}]=${elem[4]}
	esac
	case ${elem[0]} in
	    ("timeServ:")
		(( d_np[$cnt] += ${elem[2]} ));;
	esac
    done
    
    for((i=0;i<=$cnt;i++)); do
	if [ -z "${d_np[$i]}" ]; then
	    d_np[$i]=0
	fi
	if [ -z "${d_ch[$i]}" ]; then
	    d_ch[$i]=0
	fi
	if [ -z "${d_cl[$i]}" ]; then
	    d_cl[$i]=0
	fi
	if [ -z "${d_tm[$i]}" ] || [ ${d_cl[$i]} == 0 ]; then
            d_tm[$i]=0
	fi
       echo "|id cas:"$i"|NumPr:"${d_np[$i]}"|NumCl:"${d_cl[$i]}"|Tempo_Ap:"${d_tt[$i]}"|Tempo_Med:"${d_tm[$i]}"|NumChius:"${d_ch[$i]}"|"
    done

    echo "END TEST"
    
    exec 3<&-
done
