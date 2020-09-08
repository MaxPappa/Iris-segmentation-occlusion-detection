import cv2 as cv
import numpy as np
import os
import pandas as pd
from sklearn.model_selection import KFold
import matplotlib.pyplot as plt

NUM_SLICES = 5


def AllVSAll_Metrics():
	probeLBP = [filenames for (dirpath, dirnames, filenames) in os.walk('./LBP_Codes/')][0]
	probeMasks = [filenames for (dirpath, dirnames, filenames) in os.walk('./Masks/')][0]
	galleryLBP = probeLBP.copy()
	galleryMasks = probeMasks.copy()

	ids = [i[4:11] for i in probeLBP]

	df = pd.DataFrame(columns=['All-vs-All']+ids)
	# for probe subject in probeLBP
	i = 0
	for gallerySub in galleryLBP:
		dataRow = list()
		dataRow.append(gallerySub[4:11])
		for probeSub in probeLBP:
			imgProbe = cv.imread('./LBP_Codes/'+probeSub, cv.IMREAD_GRAYSCALE)
			imgGallery = cv.imread('./LBP_Codes/'+gallerySub, cv.IMREAD_GRAYSCALE)
			
			maskProbe = cv.imread('./Masks/'+probeSub[0:11]+'_NORMMASK.png', cv.IMREAD_GRAYSCALE)
			maskGallery = cv.imread('./Masks/'+gallerySub[0:11]+'_NORMMASK.png', cv.IMREAD_GRAYSCALE)

			match = lbp_match(imgProbe, imgGallery, maskProbe, maskGallery)
			dataRow.append(match)
		
		#print(dataRow)
		df.loc[i] = dataRow
		i+=1
	
	df.to_csv('./measures_AllvsAll.csv', index=False)
	return df

def plotK_ROC(folder_path, k): # path to the file containing all the measures
	dataAvg = pd.read_csv(f"{folder_path}FAR_FRR_KFOLD_{1}.csv")
	for i in range(2,k+1,1):
		dataAvg = dataAvg + pd.read_csv(f"{folder_path}FAR_FRR_KFOLD_{i}.csv")
	dataAvg = dataAvg / k
	far = dataAvg['FAR'].to_list()
	gar = dataAvg['GAR'].to_list()

	fig, ax = plt.subplots()
	ax.plot(far,gar,'r')
	plt.xlabel('FAR')
	plt.ylabel('GAR')
	plt.show()
	return dataAvg

def plotK_FAR_FRR(folder_path, k):
	dataAvg = pd.read_csv(f"{folder_path}FAR_FRR_KFOLD_{1}.csv")
	for i in range(2,k+1,1):
		dataAvg = dataAvg + pd.read_csv(f"{folder_path}FAR_FRR_KFOLD_{i}.csv")
	dataAvg = dataAvg / k
	threshold = dataAvg['threshold'].to_list()
	far = dataAvg['FAR'].to_list()
	frr = dataAvg['FRR'].to_list()
	fig, ax = plt.subplots()

	ax.plot(threshold,frr,'--b', label='FRR')
	ax.plot(threshold,far,'--r', label='FAR')
	plt.xlabel('Threshold')
	legend = ax.legend(loc='center right')
	plt.show()
	return dataAvg

def plotK_CMC(folder_path, k):
	dataAvg = pd.read_csv(f"{folder_path}CMC_KFold_{1}.csv")
	for i in range(2,k+1,1):
		dataAvg = dataAvg + pd.read_csv(f"{folder_path}CMC_KFold_{i}.csv")
	dataAvg = dataAvg / k

	rank = dataAvg['rank'].to_list()
	CMS = dataAvg['CMS'].to_list()
	fig, ax = plt.subplots()
	ax.plot(rank, CMS, 'r')
	plt.xlabel('Rank')
	plt.ylabel('CMS')
	plt.show()
	return dataAvg



def KFold_Gallery(inpath, outpath):
	data = pd.read_csv(inpath, index_col=0)
	for i in range(1,11,1): # KFold cross val. 10 times in order to achieve some nice results.
		kf = KFold(n_splits = 5, shuffle = True)
		results = next(kf.split(data), None)
		train = data.iloc[results[0]]
		test = data.iloc[results[1]]

		probeTrain = train.index
		probeTest =  train.index
		train = train[test.index]
		#test = test[train.index]
		calculate_FAR_FRR_KFold(train, test, f"{outpath}/FAR_FRR_KFOLD_{i}.csv")
		calculate_CMC_KFold(train, f"{outpath}/CMC_KFold_{i}.csv")
		print(f"\t{i}/{10} done")
		

def calculate_CMC_KFold(df, outpath):
	probe = df.columns
	gallery = df.index

	CMS = len(gallery)*[0] # list of len(gallery) zeros

	for probeId in probe:
		scores = dict()
		for galleryId in gallery:
			score = df.loc[galleryId, probeId]
			scores[galleryId] = score
		scores = sorted(scores.items(), key=lambda scores:scores[1]) # after this line, scores become a list
		#print(scores)
		#if not (scores[i][0][0:5] == probeId[0:5]):
		for i in range(0,len(scores),1):
			#print(scores[i][0][0:5], scores[i][1])
			if not (scores[i][0][0:5] == probeId[0:5]):
				continue
			CMS[i]+=1
			break
	#print(CMS)
	data = pd.DataFrame(columns=['rank','CMS'])
	CMS[0] = CMS[0]/(1.0*len(probe))
	for i in range(1,len(gallery),1):
		#print(f"CMS[i]={CMS[i]}, (1.0*len(probe))={(1.0*len(probe))}, CMS[i-1]={CMS[i-1]}")

		CMS[i] = (CMS[i]/(1.0*len(probe))) + CMS[i-1]
		newline = {'rank': i, 'CMS':CMS[i]}
		data = data.append(newline, ignore_index=True)
	data.to_csv(outpath)

def calculate_FAR_FRR_KFold(train, test, outpath):	# here train is the gallery, test is the probe
	metrics = pd.DataFrame(columns=['threshold','GAR','FAR','FRR','GRR','GA','FA','FR','GR','totGenuine','totImpostors'])

	#totGenuine = len(train.index)
	#totImpostors = (len(train.columns))*(len(train.index)) - totGenuine
	totGenuine=0
	totImpostors=0
	flag = False
	# totGenuine = len(train.columns)*(len(train.columns)-1)
	# totImpostors = len(train.columns) * (len(train.columns)-1) * len(train.index)

	for thresh in np.arange(0, 1.01, 0.01):
		FA, FR, GA, GR = 0, 0, 0, 0
		for probeId in list(train.columns): # removing the 'row=gallery/col=probe' column
			for galleryId in list(train.index): # in order to take only the gallery ids (first column of csv)
				if probeId == galleryId: continue
				match = train.loc[galleryId,probeId]
				if match <= thresh:
					if probeId[0:5] == galleryId[0:5]:
						GA+=1
					else: FA+=1
				else:
					if probeId[0:5] == galleryId[0:5]:
						FR+=1
						if not flag: totGenuine+=1
					else: 
						#print(probeId, galleryId)
						GR+=1
						if not flag: totImpostors+=1
		if not flag: flag = True
		GAR = GA / totGenuine
		FAR = FA / totImpostors
		FRR = FR / totGenuine
		GRR = GR / totImpostors
		
		newline = {'threshold':thresh, 'GAR':GAR, 'FAR':FAR, 'FRR':FRR, 'GRR':GRR, 'GA':GA, 'FA':FA, 'FR':FR, 'GR':GR, 'totGenuine':totGenuine, 'totImpostors':totImpostors}
		metrics = metrics.append(newline, ignore_index=True)
	metrics.to_csv(outpath, index=False)


def plotCMC(inpath):
	df = pd.read_csv(inpath)
	rank = df['rank'].to_list()
	CMS = df['CMS'].to_list()

	fig, ax = plt.subplots()

	ax.plot(rank, CMS, 'r')
	plt.xlabel('Rank')
	plt.ylabel('CMS')
	plt.show()

def calculateCMC(inpath, outpath):
	df = pd.read_csv(inpath)
	firstCol = 'row=gallery/col=probe'
	probe = df.columns[1:]
	gallery = df[firstCol]

	CMS = len(gallery)*[0] # list of len(gallery) zeros

	for probeId in probe:
		scores = dict()
		for gIndex in range(0,len(gallery),1):
			match = df[probeId][gIndex]
			scores[gallery[gIndex]] = match
		scores = sorted(scores.items(), key=lambda scores:scores[1])
		#print(scores)
		for i in range(0,len(scores),1):
			if not (scores[i][0][0:5] == probeId[0:5]):
				continue
			CMS[i]+=1
			break
	#print(CMS)
	data = pd.DataFrame(columns=['rank','CMS'])
	CMS[0] = CMS[0]/(1.0*len(probe))
	for i in range(1,len(gallery),1):
		CMS[i] = (CMS[i]/(1.0*len(probe))) + CMS[i-1]
		newline = {'rank': i, 'CMS':CMS[i]}
		data = data.append(newline, ignore_index=True)
		#print(newline)
	data.to_csv(outpath)



def plot_ROC(path): # path to the file containing all the measures
	df = pd.read_csv(path)
	far = df['FAR'].to_list()
	gar = df['GAR'].to_list()

	fig, ax = plt.subplots()

	ax.plot(far,gar,'r')
	plt.xlabel('FAR')
	plt.ylabel('GAR')
	plt.show()

def plot_FAR_FRR(path):
	df = pd.read_csv(path)
	threshold = df['threshold'].to_list()
	far = df['FAR'].to_list()
	frr = df['FRR'].to_list()
	fig, ax = plt.subplots()

	ax.plot(threshold,frr,'--b', label='FRR')
	ax.plot(threshold,far,'--r', label='FAR')
	plt.xlabel('Threshold')
	legend = ax.legend(loc='center right')
	plt.show()

def newEvaluation():
	probeLBP = [filenames for (dirpath, dirnames, filenames) in os.walk('./probe/lbp/')][0]
	probeMasks = [filenames for (dirpath, dirnames, filenames) in os.walk('./probe/masks/')][0]
	galleryLBP = [filenames for (dirpath, dirnames, filenames) in os.walk('./gallery/lbp/')][0]
	galleryMasks = [filenames for (dirpath, dirnames, filenames) in os.walk('./gallery/masks/')][0]

	ids = [i[4:11] for i in probeLBP]

	df = pd.DataFrame(columns=['row=gallery/col=probe']+ids)
	# for probe subject in probeLBP
	i = 0
	for gallerySub in galleryLBP:
		dataRow = list()
		dataRow.append(gallerySub[4:11])
		for probeSub in probeLBP:
			imgProbe = cv.imread('./probe/lbp/'+probeSub, cv.IMREAD_GRAYSCALE)
			imgGallery = cv.imread('./gallery/lbp/'+gallerySub, cv.IMREAD_GRAYSCALE)
			
			maskProbe = cv.imread('./probe/masks/'+probeSub[0:11]+'_NORMMASK.png', cv.IMREAD_GRAYSCALE)
			maskGallery = cv.imread('./gallery/masks/'+gallerySub[0:11]+'_NORMMASK.png', cv.IMREAD_GRAYSCALE)

			match = lbp_match(imgProbe, imgGallery, maskProbe, maskGallery)
			dataRow.append(match)
		
		print(dataRow)
		df.loc[i] = dataRow
		i+=1
	
	df.to_csv('./measures.csv', index=False)
	return df

def calculate_FAR_FRR(inpath, outpath):
	df = pd.read_csv(inpath)
	cols = df.columns
	rows = df.shape[0]
	metrics = pd.DataFrame(columns=['threshold','GAR','FAR','FRR','GRR','GA','FA','FR','GR','totGenuine','totImpostors'])
	
	firstCol = 'row=gallery/col=probe'

	totGenuine = rows
	totImpostors = (df.shape[0])*(df.shape[1]-1) - totGenuine
	flag = False

	gallery = df[firstCol]
	for thresh in np.arange(0, 1.01, 0.01):
		FA, FR, GA, GR = 0, 0, 0, 0
		for probeId in cols[1:]: # removing the 'row=gallery/col=probe' column
			for gIndex in range(0,len(gallery),1): # in order to take only the gallery ids (first column of csv)
				match = df[probeId][gIndex]
				if match <= thresh:
					if probeId[0:5] == gallery[gIndex][0:5]:
						GA+=1
					else: FA+=1
				else:
					if probeId[0:5] == gallery[gIndex][0:5]:
						FR+=1
					else: GR+=1
		GAR = GA / totGenuine
		FAR = FA / totImpostors
		FRR = FR / totGenuine
		GRR = GR / totImpostors
		
		newline = {'threshold':thresh, 'GAR':GAR, 'FAR':FAR, 'FRR':FRR, 'GRR':GRR, 'GA':GA, 'FA':FA, 'FR':FR, 'GR':GR, 'totGenuine':totGenuine, 'totImpostors':totImpostors}
		metrics = metrics.append(newline, ignore_index=True)
		print(metrics)

	metrics.to_csv(outpath, index=False)

def evaluation():
	results = list()

	probeLBP = [filenames for (dirpath, dirnames, filenames) in os.walk('./probe/lbp/')][0]
	probeMasks = [filenames for (dirpath, dirnames, filenames) in os.walk('./probe/masks/')][0]
	galleryLBP = [filenames for (dirpath, dirnames, filenames) in os.walk('./gallery/lbp/')][0]
	galleryMasks = [filenames for (dirpath, dirnames, filenames) in os.walk('./gallery/masks/')][0]

	df = pd.DataFrame(columns=['threshold','GAR','FAR','FRR','GRR','GA','FA','FR','GR','totGenuine','totImpostors'])
	for thresh in np.arange(0,1.01,0.01):
		totGenuine = 0
		totImpostors = 0
		FA, FR, GA, GR = 0, 0, 0, 0
		# for probe subject in probeLBP
		for probeSub in probeLBP:
			for gallerySub in galleryLBP:
				imgProbe = cv.imread('./probe/lbp/'+probeSub, cv.IMREAD_GRAYSCALE)
				imgGallery = cv.imread('./gallery/lbp/'+gallerySub, cv.IMREAD_GRAYSCALE)
				
				maskProbe = cv.imread('./probe/masks/'+probeSub[0:11]+'_NORMMASK.png', cv.IMREAD_GRAYSCALE)
				maskGallery = cv.imread('./gallery/masks/'+gallerySub[0:11]+'_NORMMASK.png', cv.IMREAD_GRAYSCALE)

				match = lbp_match(imgProbe, imgGallery, maskProbe, maskGallery)

				if probeSub[4:7] == gallerySub[4:7]:
					totGenuine+=1
				else: totImpostors+=1

				if match <= thresh:
					if probeSub[4:7] == gallerySub[4:7]:
						GA+=1
					else: FA+=1
				else:
					if probeSub[4:7] == gallerySub[4:7]:
						FR+=1
					else: GR+=1
				
				results.append(match)
				# if match <= 0.10:
				# 	print(f"distance between {probeSub[0:11]} and {gallerySub[0:11]} is {match}")
		GAR = GA / totGenuine
		FAR = FA / totImpostors
		FRR = FR / totGenuine
		GRR = GR / totImpostors
		
		newline = {'threshold':thresh, 'GAR':GAR, 'FAR':FAR, 'FRR':FRR, 'GRR':GRR, 'GA':GA, 'FA':FA, 'FR':FR, 'GR':GR, 'totGenuine':totGenuine, 'totImpostors':totImpostors}
		df.append(newline, ignore_index=True)
	
	df.to_csv('./FAR_FRR.csv', index=False)

	return results
	#lbp_match(imgA, imgB, maskA, maskB)

def lbp_match(imgA, imgB, maskA, maskB):
	subRows = int(imgA.shape[0] / NUM_SLICES)

	normHistA = np.zeros((256, 1), dtype=np.float32)
	normHistB = np.zeros((256, 1), dtype=np.float32)
	
	distance = 0.0

	histSize = 256	# from 0 to 255

	rows = maskA.shape[0]
	cols = maskA.shape[1]

	for i in range(0,NUM_SLICES,1):
		histogramA = cv.calcHist( [imgA[subRows*i:subRows*(i+1)]], [0], None, [histSize], [0,256] )
	#	print("histogramA calculated")
		histogramB = cv.calcHist( [imgB[subRows*i:subRows*(i+1)]], [0], None, [histSize], [0,256] )
		
		normHistA = cv.normalize(histogramA, None)
		normHistB = cv.normalize(histogramB, None)

		subMaskA = maskA[subRows*i:subRows*(i+1)]
		subMaskB = maskB[subRows*i:subRows*(i+1)]

		mr = subMaskA.shape[0]
		mc = subMaskA.shape[1]

		#print(f"round num {i}: {cv.compareHist(normHistA, normHistB, cv.HISTCMP_BHATTACHARYYA)}\n")
		distance += cv.compareHist(normHistA, normHistB, cv.HISTCMP_BHATTACHARYYA) * (1.0 - (((2*mr*mc) - (cv.countNonZero(subMaskA)+cv.countNonZero(subMaskB))) / (2.0*mr*mc)))
		#print(f"tmp summed dist is = {distance}\n")


#	print(f"final distance between A and B is: {distance/NUM_SLICES}\n")
	return distance/NUM_SLICES




if __name__ == '__main__':
	evalutation()