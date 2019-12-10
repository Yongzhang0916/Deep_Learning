from model import *
from data import *
from data3DUnetPP import *
import numpy as np
import keras
#os.environ["CUDA_VISIBLE_DEVICES"] = "0"
# myGene = trainGenerator(10,'data/membrane/train','image','label',data_gen_args,save_to_dir = None)
trainX,trainY=load_train_data()
trainX=trainX.astype('float32')
trainY=trainY.astype('float32')

print(np.mean(trainX))
print(np.std(trainX))
trainX-=np.mean(trainX);
trainX/=np.std(trainX);

#64.780174
#56.468163
# trainX=trainX/255.0;
#trainY=trainY/464.0+0.5;

threshod=0;
# trainY[trainY<=threshod]=0;
# trainY[trainY>threshod]=1;
trainY=trainY/232.0*0.5+0.5
trainY[trainY<=0.50]=0;


# trainY/=232.0
print(np.sum(trainY))
print(np.sum(trainY)/trainY.shape[0]/trainY.shape[1]/trainY.shape[2]/trainY.shape[3])

#trainY[trainY<=threshod]=0;
# trainX=trainX/255.0;

# print(trainX)
# print(trainY)
print(trainX.shape,trainY.shape)
trainX=np.reshape(trainX,trainX.shape + (1,))
trainY=np.reshape(trainY,trainY.shape + (1,))
# trainX=trainX[0:5,:,:,:,:]

model = unet3D()
model=keras.models.load_model("unet_membrane_3.hdf5",custom_objects={'bce_dice_loss': bce_dice_loss})
model_checkpoint = ModelCheckpoint('unet_membrane.hdf5', monitor='val_loss',verbose=1, save_best_only=True)
# model.fit_generator(myGene,steps_per_epoch=300,epochs=1,callbacks=[model_checkpoint])
model.fit(trainX,trainY,batch_size=4,epochs=1000,validation_split=0.2,callbacks=[model_checkpoint],class_weight='balanced')
# testGene = testGenerator("data/membrane/test")
# results = model.predict_generator(testGene,30,verbose=1)
# saveResult("data/membrane/test",results)
# print(model.predict(trainX[0:1,:,:,:,:]))