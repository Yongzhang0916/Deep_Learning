#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import print_function

import keras
import tensorflow as tf
from keras.models import Model
from keras import backend as K
from keras.layers import Input, Conv3D, concatenate, Conv3DTranspose
from keras.layers.pooling import MaxPooling3D
from keras.layers import BatchNormalization, Dropout
from keras.regularizers import l2
import os
from libtiff import TIFF
import numpy as np
import sys
from keras import callbacks
from keras.utils import multi_gpu_model
from keras import optimizers
from utils import plot_log


def mean_iou(y_true, y_pred):
    prec = []
    for t in np.arange(0.5, 1.0, 0.05):
        y_pred_ = tf.to_int32(y_pred > t)
        score, up_opt = tf.metrics.mean_iou(y_true, y_pred_, 2)
        K.get_session().run(tf.local_variables_initializer())
        with tf.control_dependencies([up_opt]):
            score = tf.identity(score)
        prec.append(score)
    return K.mean(K.stack(prec), axis=0)


# Custom loss function
def dice_coef(y_true, y_pred):
    smooth = 1.
    y_true_f = K.flatten(y_true)
    y_pred_f = K.flatten(y_pred)
    intersection = K.sum(y_true_f * y_pred_f)
    return (2. * intersection + smooth) / (K.sum(y_true_f) + K.sum(y_pred_f) + smooth)


# 样本前景背景比例不平衡 加快速度
def bce_dice_loss(y_true, y_pred):
    # return -dice_coef(y_true, y_pred)
    return 0.5 * keras.losses.binary_crossentropy(y_true, y_pred) - dice_coef(y_true, y_pred)


def standard_unit(input_tensor, stage, nb_filter, kernel_size=3):
    x = Conv3D(nb_filter, (kernel_size, kernel_size, kernel_size), activation=act, name='conv'+stage+'_1', kernel_initializer = 'he_normal', padding='same', kernel_regularizer=l2(1e-4))(input_tensor)
    x = Dropout(dropout_rate, name='dp'+stage+'_1')(x)
    x = Conv3D(nb_filter, (kernel_size, kernel_size, kernel_size), activation=act, name='conv'+stage+'_2', kernel_initializer = 'he_normal', padding='same', kernel_regularizer=l2(1e-4))(x)
    x = Dropout(dropout_rate, name='dp'+stage+'_2')(x)

    return x


def create_data(src):
    images = os.listdir(src)
    total = int(len(images) / 2)
    print(total)

    imgs = np.ndarray((total, image_x, image_y, image_z), dtype=np.uint8)
    imgs_mask = np.ndarray((total, image_x, image_y, image_z), dtype=np.uint8)

    print('Creating training images...')
    i = 0
    for image_name in images:
        if 'mask' in image_name:
            continue
        image_mask_name = image_name.split('.')[0] + "_mask.tif"

        tif = TIFF.open(src + '/' + image_name, mode='r')
        tif_mask = TIFF.open(src + '/' + image_mask_name, mode='r')

        count = 0
        imageAll = np.array([0])
        for image in tif.iter_images():
            if count == 0:
                imageAll = image
            else:
                imageAll = np.dstack((imageAll, image))
            count = count + 1

        count_mask = 0
        imageAll_mask = np.array([0])
        for image_mask in tif_mask.iter_images():
            if count_mask == 0:
                imageAll_mask = image_mask
            else:
                imageAll_mask = np.dstack((imageAll_mask, image_mask))
            count_mask = count_mask + 1

        imgs[i] = imageAll
        imgs_mask[i] = imageAll_mask
        i = i + 1

    print(imgs.shape)
    print(imgs_mask.shape)

    imgs = imgs.astype('float32')
    imgs_mask = imgs_mask.astype('float32')

    mean = np.mean(imgs)
    std = np.std(imgs)
    print('mean = ', mean)
    print('std = ', std)

    imgs -= mean
    imgs /= std

    imgs_mask /= 255.

    # add one dimension, corresponding to the input channel
    imgs = imgs[..., np.newaxis]
    imgs_mask = imgs_mask[..., np.newaxis]

    np.save('images_train.npy', imgs)
    np.save('images_mask_train.npy', imgs_mask)

    print('Creating data done!')


def load_data():
    images_train = np.load('images_train.npy')
    images_mask_train = np.load('images_mask_train.npy')
    return images_train, images_mask_train


def get_3DUnetPP(images_x, images_y, images_z, color_type=1, num_class=1,deep_supervision=False):
    nb_filter = [32, 64, 128, 256, 512]

    # Handle Dimension Ordering for different backends
    global bn_axis
    if K.image_dim_ordering() == 'tf':
      bn_axis = -1
      img_input = Input(shape=(images_x, images_y, images_z, color_type), name='main_input')
    else:
      bn_axis = 1
      img_input = Input(shape=(color_type, images_x, images_y, images_z), name='main_input')
    
    conv1_1 = standard_unit(img_input, stage='11', nb_filter=nb_filter[0])
    pool1 = MaxPooling3D((2, 2, 2), strides=(2, 2, 2), name='pool1')(conv1_1)

    conv2_1 = standard_unit(pool1, stage='21', nb_filter=nb_filter[1])
    pool2 = MaxPooling3D((2, 2, 2), strides=(2, 2, 2), name='pool2')(conv2_1)

    up1_2 = Conv3DTranspose(nb_filter[0], (2, 2, 2), strides=(2, 2, 2), name='up12', padding='same')(conv2_1)
    conv1_2 = concatenate([up1_2, conv1_1], name='merge12', axis=bn_axis)
    conv1_2 = standard_unit(conv1_2, stage='12', nb_filter=nb_filter[0])

    conv3_1 = standard_unit(pool2, stage='31', nb_filter=nb_filter[2])
    pool3 = MaxPooling3D((2, 2, 2), strides=(2, 2, 2), name='pool3')(conv3_1)

    up2_2 = Conv3DTranspose(nb_filter[1], (2, 2, 2), strides=(2, 2, 2), name='up22', padding='same')(conv3_1)
    conv2_2 = concatenate([up2_2, conv2_1], name='merge22', axis=bn_axis)
    conv2_2 = standard_unit(conv2_2, stage='22', nb_filter=nb_filter[1])

    up1_3 = Conv3DTranspose(nb_filter[0], (2, 2, 2), strides=(2, 2, 2), name='up13', padding='same')(conv2_2)
    conv1_3 = concatenate([up1_3, conv1_1, conv1_2], name='merge13', axis=bn_axis)
    conv1_3 = standard_unit(conv1_3, stage='13', nb_filter=nb_filter[0])

    conv4_1 = standard_unit(pool3, stage='41', nb_filter=nb_filter[3])
    pool4 = MaxPooling3D((2, 2, 2), strides=(2, 2, 2), name='pool4')(conv4_1)

    up3_2 = Conv3DTranspose(nb_filter[2], (2, 2, 2), strides=(2, 2, 2), name='up32', padding='same')(conv4_1)
    conv3_2 = concatenate([up3_2, conv3_1], name='merge32', axis=bn_axis)
    conv3_2 = standard_unit(conv3_2, stage='32', nb_filter=nb_filter[2])

    up2_3 = Conv3DTranspose(nb_filter[1], (2, 2, 2), strides=(2, 2, 2), name='up23', padding='same')(conv3_2)
    conv2_3 = concatenate([up2_3, conv2_1, conv2_2], name='merge23', axis=bn_axis)
    conv2_3 = standard_unit(conv2_3, stage='23', nb_filter=nb_filter[1])

    up1_4 = Conv3DTranspose(nb_filter[0], (2, 2, 2), strides=(2, 2, 2), name='up14', padding='same')(conv2_3)
    conv1_4 = concatenate([up1_4, conv1_1, conv1_2, conv1_3], name='merge14', axis=bn_axis)
    conv1_4 = standard_unit(conv1_4, stage='14', nb_filter=nb_filter[0])

    conv5_1 = standard_unit(pool4, stage='51', nb_filter=nb_filter[4])

    up4_2 = Conv3DTranspose(nb_filter[3], (2, 2, 2), strides=(2, 2, 2), name='up42', padding='same')(conv5_1)
    conv4_2 = concatenate([up4_2, conv4_1], name='merge42', axis=bn_axis)
    conv4_2 = standard_unit(conv4_2, stage='42', nb_filter=nb_filter[3])

    up3_3 = Conv3DTranspose(nb_filter[2], (2, 2, 2), strides=(2, 2, 2), name='up33', padding='same')(conv4_2)
    conv3_3 = concatenate([up3_3, conv3_1, conv3_2], name='merge33', axis=bn_axis)
    conv3_3 = standard_unit(conv3_3, stage='33', nb_filter=nb_filter[2])

    up2_4 = Conv3DTranspose(nb_filter[1], (2, 2, 2), strides=(2, 2, 2), name='up24', padding='same')(conv3_3)
    conv2_4 = concatenate([up2_4, conv2_1, conv2_2, conv2_3], name='merge24', axis=bn_axis)
    conv2_4 = standard_unit(conv2_4, stage='24', nb_filter=nb_filter[1])

    up1_5 = Conv3DTranspose(nb_filter[0], (2, 2, 2), strides=(2, 2, 2), name='up15', padding='same')(conv2_4)
    conv1_5 = concatenate([up1_5, conv1_1, conv1_2, conv1_3, conv1_4], name='merge15', axis=bn_axis)
    conv1_5 = standard_unit(conv1_5, stage='15', nb_filter=nb_filter[0])

    # method1
    nestnet_output_1 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_1',
                              kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_2)
    nestnet_output_2 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_2',
                              kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_3)
    nestnet_output_3 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_3',
                              kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_4)
    nestnet_output_4 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_4',
                              kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_5)

    # method2
    # conv_final1 = Conv3D(2, (1, 1, 1), activation=act, name='class',
    #                      kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_2)
    # conv_final2 = Conv3D(2, (1, 1, 1), activation=act, name='class',
    #                      kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_3)
    # conv_final3 = Conv3D(2, (1, 1, 1), activation=act, name='class',
    #                      kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_4)
    # conv_final4 = Conv3D(2, (1, 1, 1), activation=act, name='class',
    #                      kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv1_5)
    # nestnet_output_1 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_1',
    #                           kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv_final1)
    # nestnet_output_2 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_2',
    #                           kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv_final2)
    # nestnet_output_3 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_3',
    #                           kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv_final3)
    # nestnet_output_4 = Conv3D(num_class, (1, 1, 1), activation='sigmoid', name='output_4',
    #                           kernel_initializer='he_normal', padding='same', kernel_regularizer=l2(1e-4))(conv_final4)

    if deep_supervision: 
        model = Model(input=img_input, output=[nestnet_output_1,
                                                nestnet_output_2,
                                                nestnet_output_3,
                                                nestnet_output_4])
    else: 
        model = Model(input=img_input, output=[nestnet_output_4])

    return model


def train_model(model, args):
    print('Loading train data!')
    images_train, images_mask_train = load_data()

    # callbacks
    log = callbacks.CSVLogger(args.save_dir + '/log.csv')

    # 查看tensorboard:
    # methond1:./ python -m tensorboard.main --logdir=./
    # method22: ./ tensorboard --logdir=./
    tb = callbacks.TensorBoard(log_dir=args.save_dir + '/tensorboard-logs',
                               batch_size=args.batch_size,
                               histogram_freq=args.debug)

    checkpoint = callbacks.ModelCheckpoint(args.save_dir + '/multi-trained_model.h5',
                                           monitor='val_loss',
                                           save_best_only=True,
                                           save_weights_only=True,
                                           verbose=1,
                                           mode='min',
                                           )

    lr_decay = callbacks.LearningRateScheduler(schedule=lambda epoch: args.lr * (0.99 ** epoch))

    early_stopping = keras.callbacks.EarlyStopping(monitor='val_loss',
                                                   patience=args.patience,
                                                   verbose=0,
                                                   mode='min',
                                                   )

    paralleled_model = multi_gpu_model(model, args.gpus)

    # 断点续存
    # model = keras.models.load_model(args.save_dir + '/trained_model_old.h5',
    #                                 custom_objects={'bce_dice_loss': bce_dice_loss, 'mean_iou': mean_iou})

    # compile the model
    paralleled_model.compile(optimizer=optimizers.Adam(lr=args.lr),
                             loss=bce_dice_loss,
                             metrics=["accuracy", mean_iou])

    # Fitting model
    paralleled_model.fit(images_train, images_mask_train,
                         batch_size=args.batch_size,
                         nb_epoch=args.epochs,
                         verbose=1,
                         shuffle=True,
                         validation_split=0.2,
                         callbacks=[log, tb, lr_decay, checkpoint, early_stopping])

    # paralleled_model.load_weights(args.save_dir + '/multi-trained_model.h5')  # 加载之前训练保存的在多GPU上训练的模型参数
    # model.save(args.save_dir + '/single_gpu_model.h5')  # 保存单GPU的模型seg_model此时，保存的就是单模型参数！！

    model.save_weights(args.save_dir + '/trained_model.h5')
    print('Trained model saved to \'%s/trained_model.h5\'' % args.save_dir)

    plot_log(args.save_dir + '/log.csv', show=True)

    return model


dropout_rate = 0.5
act = "relu"
image_x = 64
image_y = 64
image_z = 32

if __name__ == "__main__":
    # setting the hyper parameters
    import argparse

    parser = argparse.ArgumentParser(description="U-Net++ on neuron images segmentation.")
    parser.add_argument('--train_dir', default='./train')
    parser.add_argument('--epochs', default=10, type=int)
    parser.add_argument('--batch_size', default=16, type=int)
    parser.add_argument('--lr', default=0.001, type=float,
                        help="Initial learning rate")
    parser.add_argument('--patience', default=5, type=int,
                        help="early stopping")
    parser.add_argument('--debug', default=0, type=int,
                        help="Save weights by TensorBoard")
    parser.add_argument('--save_dir', default='./result')
    parser.add_argument('-w', '--weights', default=None,
                        help="The path of the saved weights. Should be specified when testing")
    parser.add_argument('--gpus', default=2, type=int)
    args = parser.parse_args()
    print(args)

    # create train data
    # create_data(args.train_dir)

    if not os.path.exists(args.save_dir):
        os.makedirs(args.save_dir)

    model = get_3DUnetPP(image_x, image_y, image_z, 1)

    model.summary()
    # plot_model(model, to_file=args.save_dir + '/model.png', show_shapes=True)

    # train model
    if args.weights is not None:  # init the model weights with provided one
        model.load_weights(args.weights)

    train_model(model=model, args=args)

    # CUDA_VISIBLE_DEVICES=0,1 指定应用哪个GPU
    # python trainOptimization3DUnetPP-mutil-gpu.py input_images
