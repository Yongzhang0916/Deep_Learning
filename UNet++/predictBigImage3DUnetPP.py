#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import print_function

from train3DUnetPP_multi_gpu2 import*
from keras.models import model_from_json
import sys
import time
import cv2
import copy


def cutimage(image, patch_size, stridex, stridey, stridez):
    print("begin cut image!")
    print(image)
    tif = TIFF.open(image, mode='r')
    count = 0
    for img in tif.iter_images():
        if count == 0:
            imgAll = img
        else:
            imgAll = np.dstack((imgAll, img))
        count = count + 1
    # print(imgAll)
    print(imgAll.shape)

    # print(sum(imgAll[:, :, 1] != imgAll[:, :, 2]))
    image_x, image_y, image_z = imgAll.shape
    print(image_x, image_y, image_z)
    print(patch_size)

    # 构建图像块的索引
    if image_z - patch_size[2] <= 0:
        thres = image_z
    else:
        thres = image_z - patch_size[2]
    range_z = np.arange(0, thres, stridez)
    # range_z = np.arange(0, image_z - patch_size[2], stridez)
    range_y = np.arange(0, image_y - patch_size[1], stridey)
    range_x = np.arange(0, image_x - patch_size[0], stridex)
    print(range_z)
    print(range_y)
    print(range_x)
    print("*********************")
    if range_z[-1] != image_z - patch_size[2]:
        # range_z = np.append(range_z, image_z - patch_size[2])
        range_z = np.append(range_z, image_z)
    if range_y[-1] != image_y - patch_size[1]:
        range_y = np.append(range_y, image_y - patch_size[1])
    if range_x[-1] != image_x - patch_size[0]:
        range_x = np.append(range_x, image_x - patch_size[0])
    print(range_z)
    print(range_y)
    print(range_x)
    sz = len(range_z) * len(range_y) * len(range_x)  # 图像块的数量
    print(sz)

    res = np.zeros((sz, patch_size[0], patch_size[1], patch_size[2]))
    print(res.shape)
    imgAll=imgAll.astype('float32')
    index = 0
    for z in range_z:
        for y in range_y:
            for x in range_x:
                patch = imgAll[x:(x + patch_size[0]), y:(y + patch_size[1]), z:(z + patch_size[2])]
                # patch = patch.astype('float32')
                # patch-=np.mean(patch)
                # patch/=np.std(patch)
                # print(patch.shape)
                # Debug
                res[index,:,:,:] = copy.deepcopy(patch)
                index = index + 1
    print(res.shape)
    return res


def recoverimage(subimage, imgsize, stridex, stridey, stridez):
    print("begin recover image!")
    res = np.zeros(((imgsize[0], imgsize[1],imgsize[2])))
    # w = np.zeros(((imgsize[0], imgsize[1],imgsize[2])))

    if image_z - patch_size[2] <= 0:
        thres = image_z
    else:
        thres = image_z - patch_size[2]
    range_z = np.arange(0, thres, stridez)
    # range_z = np.arange(0, imgsize[2] - patch_size[2], stridez)
    range_y = np.arange(0, imgsize[1] - patch_size[1], stridey)
    range_x = np.arange(0, imgsize[0] - patch_size[0], stridex)
    print(range_z)
    print(range_y)
    print(range_x)
    print("*********************")

    if range_z[-1] != imgsize[2] - patch_size[2]:
        # range_z = np.append(range_z, imgsize[2] - patch_size[2])
        range_z = np.append(range_z, image_z)
    if range_y[-1] != imgsize[1] - patch_size[1]:
        range_y = np.append(range_y, imgsize[1] - patch_size[1])
    if range_x[-1] != imgsize[0] - patch_size[0]:
        range_x = np.append(range_x, imgsize[0] - patch_size[0])
    print(range_z)
    print(range_y)
    print(range_x)
    print("*********************")

    print(subimage.shape)
    index = 0
    for z in range_z:
        for y in range_y:
            for x in range_x:
                # print(res.shape)
                # print(subimage[index,:,:,:].shape)
                res[x:(x + patch_size[0]),y:(y + patch_size[1]), z:(z + patch_size[2])] = np.maximum(res[x:(x + patch_size[0]),y:(y + patch_size[1]), z:(z + patch_size[2])],subimage[index,:,:,:])
                # res[x:(x + patch_size[0]),y:(y + patch_size[1]), z:z] = np.maximum(res[x:(x + patch_size[0]),y:(y + patch_size[1]), z:z],subimage[index,:,:,:])

                index = index + 1
    # bug
    # res=res.astype('uint8')

    # tif = TIFF.open("C:\\Users\\admin\\Desktop\\zhangyong\\cut_test\\1_x_26309_y_21838_z_6616_recover.tif", mode='w')
    # for i in range(patchAll.shape[2]):
    #     img = patchAll[:, :, i]
    #     img = cv2.flip(img, 0, dst=None)  # 垂直镜像
    #     tif.write_image(img, compression=None)

    return res/1


# def predict(test_data_path,pre_data_path):
def predict(src, target, trained_model):
    imgs_test_id = list()
    # for image in os.listdir(test_data_path):
    for image in os.listdir(src):
        if 'mask' in image:
            continue
        print(image)
        img_test_id = str(image.split('.')[0])
        imgs_test_id.append(img_test_id)

        # Allpatch = cutimage(test_data_path+'/'+image, patch_size, stridex, stridey, stridez)
        Allpatch = cutimage(src + '/' + image, patch_size, stridex, stridey, stridez)
        print(Allpatch.shape)

        Allpatch = Allpatch.astype('float32')

        # Allpatch = preprocess(Allpatch)
        Allpatch = Allpatch[..., np.newaxis]

        # method1：
        # mean = 60.918564
        # std = 36.530464

        # method2:
        mean = np.mean(Allpatch)
        std = np.std(Allpatch)
        print('mean:', mean, 'std:', std)

        Allpatch -= mean
        Allpatch /= std

        model = get_3DUnetPP(image_x, image_y, image_z, 1)

        # load model
        # method1:only one GPU
        model.load_weights(trained_model)

        # method2:Mutil-GPU
        # model.load_weights(trained_model, by_name=True)
        # json_string = model.to_json()
        # model = model_from_json(json_string)

        Allpatch_pre = model.predict(Allpatch,
                                     batch_size=BS,
                                     verbose=1)

        print(Allpatch_pre.shape)
        print(np.max(Allpatch_pre), np.min(Allpatch_pre))
        Allpatch_pre = Allpatch_pre[:, :, :, :, 0]
        # print(Allpatch_pre.shape)

        imgsize = (imgtest_x, imgtest_y, imgtest_z)
        # print(imgsize)
        pre_result = recoverimage(Allpatch_pre,imgsize,stridex,stridey,stridez)
        # print(pre_result.shape)

        pre_result = (pre_result*255).astype(np.uint8)

        # tif_pre = TIFF.open(pre_data_path + '/' + str(img_test_id) + '_unetpp.tif', mode='w')
        tif_pre = TIFF.open(target + '/' + str(img_test_id) + '_unetpp.tif', mode='w')
        for i in range(pre_result.shape[2]):
            img = pre_result[:, :, i]
            img = cv2.flip(img, 0, dst=None)  # 垂直镜像
            tif_pre.write_image(img, compression=None)


if __name__ == "__main__":
    argv = sys.argv;
    print(argv)

    imgtest_x = int(argv[4])
    imgtest_y = int(argv[5])
    imgtest_z = int(argv[6])

    # stridex = int(argv[7])
    # stridey = int(argv[8])
    # stridez = int(argv[9])

    stridex = 48
    stridey = 48
    stridez = 24

    patch_size = (image_x, image_y, image_z)
    BS = 32

    start_predict = time.clock()

    predict(argv[1], argv[2], argv[3])

    end_predict = time.clock()
    print('start predict time:', start_predict)
    print('end predict time:', end_predict)
    print('used time:', (end_predict - start_predict), 's')

    # CUDA_VISIBLE_DEVICES=0,1 指定应用哪个GPU
    # python predictBigImage3DUnetPP.py input_image predict_image trained_model.h5 256 256 128
