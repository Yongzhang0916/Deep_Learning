#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import print_function

from train3DUnetPP import*
from keras.models import model_from_json
import time
import sys
import cv2


def create_test_data(src):
    # test_data_path = os.path.join(data_path,'test')
    images = os.listdir(src)
    total = len(images)

    imgs_test = np.ndarray((total, imgtest_x, imgtest_y, imgtest_z), dtype=np.uint8)
    #imgs_test_id = np.ndarray((total,), dtype = np.str)
    imgs_test_id = list()

    print('Creating testing images')

    i = 0
    for image_name in images:
        tif = TIFF.open(src + '/' + image_name, mode = 'r')
        img_test_id = str(image_name.split('.')[0])

        count_test = 0
        imageAll_test = np.array([0])
        for image in tif.iter_images():
            if count_test == 0:
                imageAll_test = image
            else:
                imageAll_test = np.dstack((imageAll_test,image))
            count_test = count_test +1

        imgs_test[i] = imageAll_test
        imgs_test_id.append(img_test_id)
        i = i+1

    print(imgs_test.shape)
    print(imgs_test_id)

    imgs_test = imgs_test.astype('float32')
    imgs_test = imgs_test[..., np.newaxis]

    # method1：
    # images_train = load_data()
    # mean = np.mean(images_train)
    # std = np.std(images_train)

    # method2
    mean = np.mean(imgs_test)
    std = np.std(imgs_test)
    print('mean:', mean, 'std:', std)

    imgs_test -= mean
    imgs_test /= std

    np.save('images_test.npy', imgs_test)
    np.save('images_test_id.npy', imgs_test_id)

    print('Creating testing images done!')


def load_test_data():
    images_test = np.load('images_test.npy')
    images_test_id = np.load('images_test_id.npy')
    return images_test, images_test_id


def predict(target, trained_model):
    images_test, images_test_id = load_test_data()

    model = get_3DUnetPP(imgtest_x, imgtest_y, imgtest_z, 1)

    # load model
    # method1:only one GPU
    model.load_weights(trained_model)

    # method2:Mutil-GPU
    # model.load_weights(trained_model, by_name=True)
    # json_string = model.to_json()
    # model = model_from_json(json_string)

    images_mask_test = model.predict(images_test, batch_size=BS,verbose=1)

    print('Saving predicted masks to files...')
    for image, image_id in zip(images_mask_test, images_test_id):
        image = (image[:, :, :, 0] * 255.).astype(np.uint8)
        # print(image.shape)
        # print(image)

        tif = TIFF.open(os.path.join(target, str(image_id) + '_unetpp.tif'), mode='w')
        for i in range(image.shape[2]):
            img = image[:, :, i]
            img = cv2.flip(img, 0, dst=None)  # 垂直镜像
            tif.write_image(img, compression=None)


if __name__ == '__main__':
    argv = sys.argv;
    print(argv)

    imgtest_x = int(argv[4])
    imgtest_y = int(argv[5])
    imgtest_z = int(argv[6])

    BS = int(argv[7])

    start_predict = time.clock()

    # create_test_data(argv[1])
    predict(argv[2], argv[3])

    end_predict = time.clock()
    print('start predict time:', start_predict)
    print('end predict time:', end_predict)
    print('used time:', (end_predict - start_predict), 's')

    # CUDA_VISIBLE_DEVICES=0,1 指定应用哪个GPU
    # python predictSubImage3DUnetPP.py input_image predict_image trained_model.h5 64 64 32 BS